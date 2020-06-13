#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "elf.h"

extern char data[]; // defined by kernel.ld
pde_t *kpgdir;      // for use in scheduler()

// Set up CPU's kernel segment descriptors.
// Run once on entry on each CPU.
void seginit(void)
{
  struct cpu *c;

  // Map "logical" addresses to virtual addresses using identity map.
  // Cannot share a CODE descriptor for both kernel and user
  // because it would have to have DPL_USR, but the CPU forbids
  // an interrupt from CPL=0 to DPL=3.
  c = &cpus[cpuid()];
  c->gdt[SEG_KCODE] = SEG(STA_X | STA_R, 0, 0xffffffff, 0);
  c->gdt[SEG_KDATA] = SEG(STA_W, 0, 0xffffffff, 0);
  c->gdt[SEG_UCODE] = SEG(STA_X | STA_R, 0, 0xffffffff, DPL_USER);
  c->gdt[SEG_UDATA] = SEG(STA_W, 0, 0xffffffff, DPL_USER);
  lgdt(c->gdt, sizeof(c->gdt));
}

// Return the address of the PTE in page table pgdir
// that corresponds to virtual address va.  If alloc!=0,
// create any required page table pages.
static pte_t *
walkpgdir(pde_t *pgdir, const void *va, int alloc)
{
  pde_t *pde;
  pte_t *pgtab;

  pde = &pgdir[PDX(va)];
  if (*pde & PTE_P)
  {
    pgtab = (pte_t *)P2V(PTE_ADDR(*pde));
  }
  else
  {
    if (!alloc || (pgtab = (pte_t *)kalloc()) == 0)
    {
      // cprintf("returning 0, got pte %x with val:%x \n", pde, *pde);
      return 0;
    }
    // Make sure all those PTE_P bits are zero.
    memset(pgtab, 0, PGSIZE);
    // The permissions here are overly generous, but they can
    // be further restricted by the permissions in the page table
    // entries, if necessary.
    *pde = V2P(pgtab) | PTE_P | PTE_W | PTE_U;
  }
  return &pgtab[PTX(va)];
}

// Create PTEs for virtual addresses starting at va that refer to
// physical addresses starting at pa. va and size might not
// be page-aligned.
static int
mappages(pde_t *pgdir, void *va, uint size, uint pa, int perm)
{
  char *a, *last;
  pte_t *pte;

  a = (char *)PGROUNDDOWN((uint)va);
  last = (char *)PGROUNDDOWN(((uint)va) + size - 1);
  for (;;)
  {
    if ((pte = walkpgdir(pgdir, a, 1)) == 0)
      return -1;
    if (*pte & PTE_P)
      panic("remap");
    *pte = pa | perm | PTE_P;
    if (a == last)
      break;
    a += PGSIZE;
    pa += PGSIZE;
  }
  return 0;
}

// There is one page table per process, plus one that's used when
// a CPU is not running any process (kpgdir). The kernel uses the
// current process's page table during system calls and interrupts;
// page protection bits prevent user code from using the kernel's
// mappings.
//
// setupkvm() and exec() set up every page table like this:
//
//   0..KERNBASE: user memory (text+data+stack+heap), mapped to
//                phys memory allocated by the kernel
//   KERNBASE..KERNBASE+EXTMEM: mapped to 0..EXTMEM (for I/O space)
//   KERNBASE+EXTMEM..data: mapped to EXTMEM..V2P(data)
//                for the kernel's instructions and r/o data
//   data..KERNBASE+PHYSTOP: mapped to V2P(data)..PHYSTOP,
//                                  rw data + free physical memory
//   0xfe000000..0: mapped direct (devices such as ioapic)
//
// The kernel allocates physical memory for its heap and for user memory
// between V2P(end) and the end of physical memory (PHYSTOP)
// (directly addressable from end..P2V(PHYSTOP)).

// This table defines the kernel's mappings, which are present in
// every process's page table.
static struct kmap
{
  void *virt;
  uint phys_start;
  uint phys_end;
  int perm;
} kmap[] = {
    {(void *)KERNBASE, 0, EXTMEM, PTE_W},            // I/O space
    {(void *)KERNLINK, V2P(KERNLINK), V2P(data), 0}, // kern text+rodata
    {(void *)data, V2P(data), PHYSTOP, PTE_W},       // kern data+memory
    {(void *)DEVSPACE, DEVSPACE, 0, PTE_W},          // more devices
};

// Set up kernel part of a page table.
pde_t *
setupkvm(void)
{
  pde_t *pgdir;
  struct kmap *k;

  if ((pgdir = (pde_t *)kalloc()) == 0)
    return 0;
  memset(pgdir, 0, PGSIZE);
  if (P2V(PHYSTOP) > (void *)DEVSPACE)
    panic("PHYSTOP too high");
  for (k = kmap; k < &kmap[NELEM(kmap)]; k++)
    if (mappages(pgdir, k->virt, k->phys_end - k->phys_start,
                 (uint)k->phys_start, k->perm) < 0)
    {
      freevm(pgdir);
      return 0;
    }
  return pgdir;
}

// Allocate one page table for the machine for the kernel address
// space for scheduler processes.
void kvmalloc(void)
{
  kpgdir = setupkvm();
  switchkvm();
}

// Switch h/w page table register to the kernel-only page table,
// for when no process is running.
void switchkvm(void)
{
  lcr3(V2P(kpgdir)); // switch to the kernel page table
}

// Switch TSS and h/w page table to correspond to process p.
void switchuvm(struct proc *p)
{
  if (p == 0)
    panic("switchuvm: no process");
  if (p->kstack == 0)
    panic("switchuvm: no kstack");
  if (p->pgdir == 0)
    panic("switchuvm: no pgdir");

  pushcli();
  mycpu()->gdt[SEG_TSS] = SEG16(STS_T32A, &mycpu()->ts,
                                sizeof(mycpu()->ts) - 1, 0);
  mycpu()->gdt[SEG_TSS].s = 0;
  mycpu()->ts.ss0 = SEG_KDATA << 3;
  mycpu()->ts.esp0 = (uint)p->kstack + KSTACKSIZE;
  // setting IOPL=0 in eflags *and* iomb beyond the tss segment limit
  // forbids I/O instructions (e.g., inb and outb) from user space
  mycpu()->ts.iomb = (ushort)0xFFFF;
  ltr(SEG_TSS << 3);
  lcr3(V2P(p->pgdir)); // switch to process's address space
  popcli();
}

// Load the initcode into address 0 of pgdir.
// sz must be less than a page.
void inituvm(pde_t *pgdir, char *init, uint sz)
{
  char *mem;

  if (sz >= PGSIZE)
    panic("inituvm: more than a page");
  mem = kalloc();
  memset(mem, 0, PGSIZE);
  mappages(pgdir, 0, PGSIZE, V2P(mem), PTE_W | PTE_U);
  memmove(mem, init, sz);
}

// Load a program segment into pgdir.  addr must be page-aligned
// and the pages from addr to addr+sz must already be mapped.
int loaduvm(pde_t *pgdir, char *addr, struct inode *ip, uint offset, uint sz)
{
  uint i, pa, n;
  pte_t *pte;

  if ((uint)addr % PGSIZE != 0)
    panic("loaduvm: addr must be page aligned");
  for (i = 0; i < sz; i += PGSIZE)
  {
    if ((pte = walkpgdir(pgdir, addr + i, 0)) == 0)
      panic("loaduvm: address should exist");
    pa = PTE_ADDR(*pte);
    if (sz - i < PGSIZE)
      n = sz - i;
    else
      n = PGSIZE;
    if (readi(ip, P2V(pa), offset + i, n) != n)
      return -1;
  }
  return 0;
}

int getPagePAddr(int userPageVAddr, pde_t *pgdir)
{
  pte_t *pte;

  pte = walkpgdir(pgdir, (int *)userPageVAddr, 0);
  if (!pte) //uninitialized page table
  {
    return -1;
  }
  return PTE_ADDR(*pte);
}

void fixPagedOutPTE(int userPageVAddr, pde_t *pgdir)
{
  pte_t *pte;
  pte = walkpgdir(pgdir, (int *)userPageVAddr, 0);
  if (!pte)
    panic("PTE of swapped page is missing");
  *pte |= PTE_PG;
  *pte &= ~PTE_P;
  *pte &= PTE_FLAGS(*pte);    //clear junk physical address
  lcr3(V2P(myproc()->pgdir)); //refresh CR3 register
}

/**
 * 
 * Getting the virtual address, set the PTE and update the CR3 register
 *   
 * */
void fixPagedInPTE(int userPageVAddr, int pagePAddr, pde_t *pgdir)
{
  pte_t *pte;
  // Get the physical address
  pte = walkpgdir(pgdir, (int *)userPageVAddr, 0);

  if (!pte)
  {
    panic("PTE of swapped page is missing");
  }
  if (*pte & PTE_P)
  {
    panic("PAGE IN REMAP!");
  }
  *pte |= PTE_P | PTE_W | PTE_U; //Turn on needed bits

  *pte &= ~PTE_PG; //Turn off secondary storage bits

  *pte |= pagePAddr; //Map PTE to the new Page

  lcr3(V2P(myproc()->pgdir)); //refresh CR3 register
}

int pageIsInFile(int userPageVAddr, pde_t *pgdir)
{
  pte_t *pte;
  pte = walkpgdir(pgdir, (char *)userPageVAddr, 0);
  return (*pte & PTE_PG); //PAGE IS IN FILE
}

int getSCFIFO()
{
  pte_t *pte;
  int i = 0;
  int pageIndex;
  uint loadOrder;
#ifdef VERBOSE_PRINT
  cprintf("getting page using SCFIFO \n");
#endif

recheck:
  pageIndex = -1;
  loadOrder = 0xFFFFFFFF;
  for (i = 0; i < MAX_PSYC_PAGES; i++)
  {
    if ((myproc()->ramCtrlr[i].state == USED) && (myproc()->ramCtrlr[i].loadOrder <= loadOrder))
    {
      #ifdef VERBOSE_PRINT
        cprintf("index %d loadOrder is %d\n",i,myproc()->ramCtrlr[i].loadOrder);
      #endif
      pageIndex = i;
      loadOrder = myproc()->ramCtrlr[i].loadOrder;

    }
  }

  // Check to see if the ram page is in accessed mode or not
  pte = walkpgdir(myproc()->ramCtrlr[pageIndex].pgdir, (char *)myproc()->ramCtrlr[pageIndex].userPageVAddr, 0);

  if (*pte & PTE_A)
  {
    // If it is accessed, set it off, set the current load order and retest the SCFIFO.
    *pte &= ~PTE_A;
    myproc()->ramCtrlr[pageIndex].loadOrder = myproc()->loadOrderCounter++;
    #ifdef VERBOSE_PRINT
      cprintf("index %d is already accessed. going to recheck\n", pageIndex);
    #endif
    goto recheck;
  }
  #ifdef VERBOSE_PRINT
    cprintf("returning %d \n", pageIndex);
  #endif
  return pageIndex;
}

int getAQ()
{
  pte_t *pte;
  int i = 0;
  int pageIndex = -1;
  uint foundqueuePos = 0;
#ifdef VERBOSE_PRINT
  cprintf("getting page using AQ \n");
#endif
  for (i = 0; i < MAX_PSYC_PAGES; i++)
  {

    if ((myproc()->ramCtrlr[i].state == USED) && (myproc()->ramCtrlr[i].queuePos >= foundqueuePos))
    {
      pageIndex = i;
      foundqueuePos = myproc()->ramCtrlr[i].queuePos;
      cprintf("queuePos for %x is: %d\n",myproc()->ramCtrlr[i].userPageVAddr,myproc()->ramCtrlr[i].queuePos);
      cprintf("page index: %d\n", find_index_from_queuePos(myproc()->ramCtrlr[i].queuePos));
    }
  }
  pte = walkpgdir(myproc()->ramCtrlr[pageIndex].pgdir, (char *)myproc()->ramCtrlr[pageIndex].userPageVAddr, 0);

  if (*pte & PTE_A)
  {
    *pte &= ~PTE_A;
    //very time a page gets accessed, it should switch places with the page
    //preceding it the queue (unless it is already in the first place)
    if (foundqueuePos < myproc()->queuePos) // 1?
    {
      
      int preceding = find_index_from_queuePos(foundqueuePos + 1);
      if (preceding != -1)
      {
        myproc()->ramCtrlr[preceding].queuePos--;
      }
      myproc()->ramCtrlr[pageIndex].queuePos++;
    }
  }
  cprintf("returning page number %d\n",pageIndex);
  return pageIndex;
}

int find_index_from_queuePos(uint pos_to_find)
{
  int i;
  for (i = 0; i < MAX_PSYC_PAGES; i++)
  {
    if (myproc()->ramCtrlr[i].queuePos == pos_to_find)
      return i;
  }
  return -1;
}

/**
 * NFU + AGING
 * get the index for the page we should swap when using the NFU method.
 * When a page is accessed we update the counter by shifting it right by one, and we add a 1 on the left.
 * If it isn't accessed (& PTE_A == 0) then we just shift by one.
 * 
 * The counter returned is the page with the lowest counter
 * */
int getNFU()
{
  int i;
  int pageIndex = -1;
  uint minAccess = 0xffffffff; // All bits lit.
#ifdef VERBOSE_PRINT
  cprintf("getting page using NFU \n");
#endif
  for (i = 0; i < MAX_PSYC_PAGES; i++)
  {
    if ((myproc()->ramCtrlr[i].state == USED) &&
        (myproc()->ramCtrlr[i].accessCount <= minAccess)) // Notice the <=, so that even if some page is all 1's, we get him and not -1.
    {
      minAccess = myproc()->ramCtrlr[i].accessCount;
      pageIndex = i;
    }
  }
  cprintf("Getting index %d\n", pageIndex);
  return pageIndex;
}

unsigned int bit_count_simple(unsigned int value)
{
  unsigned int count = 0;
  while (value > 0)
  {                       // until all bits are zero
    if ((value & 1) == 1) // check lower bit
      count++;
    value >>= 1; // shift bits, removing lower bit
  }
  return count;
}

/**
 * Least accessed page + AGING
 * get the index for the page we should swap when using the NFU method.
 * When a page is accessed we update the counter by shifting it right by one, and we add a 1 on the left.
 * If it isn't accessed (& PTE_A == 0) then we just shift by one.
 * 
 * The counter returned is the page with the lowest counter
 * */
int getLAPA()
{
  int i;
  int pageIndex = -1;
  //  uint minAccess = 0xffffffff;
  uint shifted = 33; // 32 bits + 1.
#ifdef VERBOSE_PRINT
  cprintf("getting page using LAPA \n");
#endif
  for (i = 0; i < MAX_PSYC_PAGES; i++)
  {
    if ((myproc()->ramCtrlr[i].state == USED) && (bit_count_simple(myproc()->ramCtrlr[i].accessCount) <= shifted))
    {
      shifted = bit_count_simple(myproc()->ramCtrlr[i].accessCount);
      //minAccess = myproc()->ramCtrlr[i].accessCount;
      pageIndex = i;
    }
  }
#ifdef VERBOSE_PRINT
  cprintf("returning page %d \n", pageIndex);
#endif
  cprintf("returning %d \n", pageIndex);
  return pageIndex;
}

int getPageOutIndex()
{
#ifdef VERBOSE_PRINT

  cprintf("Got get page\n");
#endif

#ifdef SCFIFO
  return getSCFIFO();
#elif LAPA
  return getLAPA();
#elif NFU
  return getNFU();
#elif AQ
  return getAQ();
#elif NONE
  return 1;
#else
  panic("Unrecognized paging machanism");
#endif
}

void update_proc_counters(struct proc *p)
{
  pte_t *pte;
  int i;
  for (i = 0; i < MAX_PSYC_PAGES; i++)
  {
    if (p->ramCtrlr[i].state == USED)
    {
      pte = walkpgdir(p->ramCtrlr[i].pgdir, (char *)p->ramCtrlr[i].userPageVAddr, 0);
      p->ramCtrlr[i].accessCount = (p->ramCtrlr[i].accessCount >> 1);
      if (*pte & PTE_A)
      {
        *pte &= ~PTE_A; // turn off PTE_A flag
        p->ramCtrlr[i].accessCount |= (1 << 31);
/*#ifdef AQ
        cprintf("accessed page %d\n\n\n",i);
        uint queuePos = p->ramCtrlr[i].queuePos;
        int index = find_index_from_queuePos(queuePos);
        if (queuePos < myproc()->queuePos) 
        {
      
          int preceding = find_index_from_queuePos(queuePos + 1);
          if (preceding != -1)
          {
            myproc()->ramCtrlr[preceding].queuePos--;
          }
          myproc()->ramCtrlr[index].queuePos++;
        }
#endif*/
      }
    }
  }
}

int getFreeRamCtrlrIndex()
{
  if (myproc() == 0)
    return -1;
  int i;
  for (i = 0; i < MAX_PSYC_PAGES; i++)
  {
    if (myproc()->ramCtrlr[i].state == NOTUSED)
      return i;
  }
  return -1; //NO ROOM IN RAMCTRLR
}

static char buff[PGSIZE]; //buffer used to store swapped page in getPageFromFile method

int getPageFromFile(int cr2)
{
  // Page fault
  myproc()->faultCounter++;
  int userPageVAddr = PGROUNDDOWN(cr2);

  // Allocate page and see if we can place it
  char *newPg = kalloc();
  memset(newPg, 0, PGSIZE);
  int outIndex = getFreeRamCtrlrIndex();
  lcr3(V2P(myproc()->pgdir)); //refresh CR3 register
  if (outIndex >= 0)
  {
    // We have a free index in our ram pages
    fixPagedInPTE(userPageVAddr, V2P(newPg), myproc()->pgdir);
    readPageFromFile(myproc(), outIndex, userPageVAddr, (char *)userPageVAddr);
    return 1; //Operation was successful
  }
  // No room in pages, find the page to swap
  myproc()->countOfPagedOut++;
  outIndex = getPageOutIndex(); //select a page to swap to file

  struct pagecontroller outPage = myproc()->ramCtrlr[outIndex];
  fixPagedInPTE(userPageVAddr, V2P(newPg), myproc()->pgdir);
  readPageFromFile(myproc(), outIndex, userPageVAddr, buff); //automatically adds to ramctrlr
  int outPagePAddr = getPagePAddr(outPage.userPageVAddr, outPage.pgdir);
  memmove(newPg, buff, PGSIZE);
  writePageToFile(myproc(), outPage.userPageVAddr, outPage.pgdir);
  fixPagedOutPTE(outPage.userPageVAddr, outPage.pgdir);
  char *v = P2V(outPagePAddr);
  kfree(v); //free swapped page

  return 1;
}

void addToRamCtrlr(pde_t *pgdir, uint v_addr)
{
  int freeLocation = getFreeRamCtrlrIndex();
  if (freeLocation == -1)
    panic("no free location, should be one.");
  myproc()->ramCtrlr[freeLocation].state = USED;
  myproc()->ramCtrlr[freeLocation].pgdir = pgdir;
  myproc()->ramCtrlr[freeLocation].userPageVAddr = v_addr;
  myproc()->ramCtrlr[freeLocation].loadOrder = myproc()->loadOrderCounter++;
  myproc()->ramCtrlr[freeLocation].accessCount = 0;
  myproc()->ramCtrlr[freeLocation].queuePos = myproc()->queuePos++;
#ifdef LAPA
  myproc()->ramCtrlr[freeLocation].accessCount = 0xffffffff;
#else
  myproc()->ramCtrlr[freeLocation].accessCount = 0;
#endif
}

void swap(pde_t *pgdir, uint userPageVAddr)
{
  myproc()->countOfPagedOut++;
  int outIndex = getPageOutIndex();

  int outPagePAddr = getPagePAddr(myproc()->ramCtrlr[outIndex].userPageVAddr, myproc()->ramCtrlr[outIndex].pgdir);
  writePageToFile(myproc(), myproc()->ramCtrlr[outIndex].userPageVAddr, myproc()->ramCtrlr[outIndex].pgdir);
  char *v = P2V(outPagePAddr);
  kfree(v); //free swapped page
  myproc()->ramCtrlr[outIndex].state = NOTUSED;
  fixPagedOutPTE(myproc()->ramCtrlr[outIndex].userPageVAddr, myproc()->ramCtrlr[outIndex].pgdir);
  addToRamCtrlr(pgdir, userPageVAddr);
}

int isNONEpolicy()
{
#ifdef NONE
  return 1;
#endif
  return 0;
}

// Allocate page tables and physical memory to grow process from oldsz to
// newsz, which need not be page aligned.  Returns new size or 0 on error.
int allocuvm(pde_t *pgdir, uint oldsz, uint newsz)
{
  char *mem;
  uint a;
  if (newsz >= KERNBASE)
    return 0;
  if (newsz < oldsz)
    return oldsz;

  if (!isNONEpolicy())
  {
    if (PGROUNDUP(newsz) / PGSIZE > MAX_TOTAL_PAGES && myproc()->pid > 2)
    {
      cprintf("proc is too big\n", PGROUNDUP(newsz) / PGSIZE);
      return 0;
    }
  }

  a = PGROUNDUP(oldsz);
  int i = 0; //debugging
  for (; a < newsz; a += PGSIZE)
  {
    mem = kalloc();
    i++;
    if (mem == 0)
    {
      cprintf("allocuvm out of memory\n");
      deallocuvm(pgdir, newsz, oldsz);
      return 0;
    }
    memset(mem, 0, PGSIZE);
    mappages(pgdir, (char *)a, PGSIZE, V2P(mem), PTE_W | PTE_U);
    if (!isNONEpolicy() && myproc()->pid > 2)
    {
#ifdef VERBOSE_PRINT
      cprintf("PGROUNGUP %d oldsz :%d newsz:%d , i %d \n", PGROUNDUP(oldsz), oldsz, newsz, i);
#endif
      if (getFreeRamCtrlrIndex() == -1)
      {
        swap(pgdir, a);
      }
      else //there's room
      {
        addToRamCtrlr(pgdir, a);
      }
    }
  }
#ifdef VERBOSE_PRINT

  cprintf("Returning newsz : %d\n", newsz);
#endif
  return newsz;
}

//This must use userVaddress+pgdir addresses!
//(The proc has identical vAddresses on different page directories until exec finish executing)
void removeFromRamCtrlr(uint userPageVAddr, pde_t *pgdir)
{
  if (myproc() == 0)
    return;
  int i;
  for (i = 0; i < MAX_PSYC_PAGES; i++)
  {
    if (myproc()->ramCtrlr[i].state == USED && myproc()->ramCtrlr[i].userPageVAddr == userPageVAddr && myproc()->ramCtrlr[i].pgdir == pgdir)
    {
      myproc()->ramCtrlr[i].state = NOTUSED;
      return;
    }
  }
}

void removeFromFileCtrlr(uint userPageVAddr, pde_t *pgdir)
{
  if (myproc() == 0)
    return;
  int i;
  for (i = 0; i < MAX_TOTAL_PAGES - MAX_PSYC_PAGES; i++)
  {
    if (myproc()->fileCtrlr[i].state == USED && myproc()->fileCtrlr[i].userPageVAddr == userPageVAddr && myproc()->fileCtrlr[i].pgdir == pgdir)
    {
      myproc()->fileCtrlr[i].state = NOTUSED;
      return;
    }
  }
}
// Deallocate user pages to bring the process size from oldsz to
// newsz.  oldsz and newsz need not be page-aligned, nor does newsz
// need to be less than oldsz.  oldsz can be larger than the actual
// process size.  Returns the new process size.
int deallocuvm(pde_t *pgdir, uint oldsz, uint newsz)
{
  pte_t *pte;
  uint a, pa;

  if (newsz >= oldsz)
    return oldsz;

  a = PGROUNDUP(newsz);
  for (; a < oldsz; a += PGSIZE)
  {
    pte = walkpgdir(pgdir, (char *)a, 0);
    if (!pte)
      a = PGADDR(PDX(a) + 1, 0, 0) - PGSIZE;
    else if ((*pte & PTE_P) != 0)
    {
      pa = PTE_ADDR(*pte);
      if (pa == 0)
        panic("kfree");
      uint cow_reference_count = get_count(pa);
      char *v = P2V(pa);
      //not deallocate pages that other processes have a reference to
      if (cow_reference_count <= 1)
      {
        kfree(v);
        if (!isNONEpolicy())
          removeFromRamCtrlr(a, pgdir);
      }
      else
      {
        decrement_count(pa);
      }
      *pte = 0;
    }
  }
  return newsz;
}

// Free a page table and all the physical memory pages
// in the user part.
void freevm(pde_t *pgdir)
{
  uint i;

  if (pgdir == 0)
    panic("freevm: no pgdir");
  deallocuvm(pgdir, KERNBASE, 0);
  for (i = 0; i < NPDENTRIES; i++)
  {
    if (pgdir[i] & PTE_P)
    {
      char *v = P2V(PTE_ADDR(pgdir[i]));
      kfree(v);
    }
  }
  kfree((char *)pgdir);
}

// Clear PTE_U on a page. Used to create an inaccessible
// page beneath the user stack.
void clearpteu(pde_t *pgdir, char *uva)
{
  pte_t *pte;

  pte = walkpgdir(pgdir, uva, 0);
  if (pte == 0)
    panic("clearpteu");
  *pte &= ~PTE_U;
}

// Given a parent process's page table, create a copy
// of it for a child.
pde_t *
copyuvm(pde_t *pgdir, uint sz)
{
  pde_t *d;
  pte_t *pte;
  uint pa, i, flags;
  char *mem;

  if ((d = setupkvm()) == 0)
    return 0;
  for (i = 0; i < sz; i += PGSIZE)
  {
    if ((pte = walkpgdir(pgdir, (void *)i, 0)) == 0)
      panic("copyuvm: pte should exist");
    if (!(*pte & PTE_P) && !(*pte & PTE_PG))
      panic("copyuvm: page not present");
    pa = PTE_ADDR(*pte);
    flags = PTE_FLAGS(*pte);
    if ((mem = kalloc()) == 0)
      goto bad;
    memmove(mem, (char *)P2V(pa), PGSIZE);
    if (mappages(d, (void *)i, PGSIZE, V2P(mem), flags) < 0)
    {
      kfree(mem);
      goto bad;
    }
  }
  return d;

bad:
  freevm(d);
  return 0;
}

pte_t *cowuvm(pde_t *pgdir, uint sz)
{
  /*
    Converts each writeable page table entry in the parent and child to read-only and PTE_COW.
    You will need to also shoot down the TLB entries for any virtual addresses you changed
    For each page that is shared copy-on-write, you will need to increment the refernce count.

    Note that your cowuvm implementation will not need to allocate new page frames using kalloc() 
    for the process. Rather, this will be done lazily in the page fault handler. 

  */
  pde_t *d;
  pte_t *pte;
  uint pa, i, flags;

  if ((d = setupkvm()) == 0)
    return 0;
  for (i = 0; i < sz; i += PGSIZE)
  {
    if ((pte = walkpgdir(pgdir, (void *)i, 0)) == 0)
      panic("copyuvm: pte should exist");
    if (!(*pte & PTE_P) && !(*pte & PTE_PG))
      panic("copyuvm: page not present");

    *pte |= PTE_COW; //cow flag on
    *pte &= ~PTE_W;  //read only
    pa = PTE_ADDR(*pte);

    flags = PTE_FLAGS(*pte);

    if (mappages(d, (void *)i, PGSIZE, pa, flags) < 0)
      goto bad;

    increment_count(pa);
  }
  lcr3(V2P(pgdir)); //flush TLB
  return d;

bad:
  freevm(d);
  lcr3(V2P(pgdir)); //flush TLB
  return 0;
}

//PAGEBREAK!
// Map user virtual address to kernel address.
char *
uva2ka(pde_t *pgdir, char *uva)
{
  pte_t *pte;

  pte = walkpgdir(pgdir, uva, 0);
  if ((*pte & PTE_P) == 0)
    return 0;
  if ((*pte & PTE_U) == 0)
    return 0;
  return (char *)P2V(PTE_ADDR(*pte));
}

// Copy len bytes from p to user address va in page table pgdir.
// Most useful when pgdir is not the current page table.
// uva2ka ensures this only works for PTE_U pages.
int copyout(pde_t *pgdir, uint va, void *p, uint len)
{
  char *buf, *pa0;
  uint n, va0;

  buf = (char *)p;
  while (len > 0)
  {
    va0 = (uint)PGROUNDDOWN(va);
    pa0 = uva2ka(pgdir, (char *)va0);
    if (pa0 == 0)
      return -1;
    n = PGSIZE - (va - va0);
    if (n > len)
      n = len;
    memmove(pa0 + (va - va0), buf, n);
    len -= n;
    buf += n;
    va = va0 + PGSIZE;
  }
  return 0;
}

void pagefault()
{

  struct proc *p = myproc();
  uint virt_addr = rcr2(); //the faulting address is available by calling rcr2()
  pte_t *pte;
  char *new_mem;

  char *addr = (char *)PGROUNDDOWN((uint)virt_addr);
  /*
walkpgdir
Return the address of the PTE in page table pgdir
that corresponds to virtual address va.  If alloc!=0,
create any required page table pages.
*/
  if (virt_addr >= KERNBASE || (pte = walkpgdir(p->pgdir, addr, 0)) == 0 ||
      !(*pte & PTE_U))
  {
    cprintf("pid %d %s: pagefault: invalid address %d\n", p->pid, p->name, virt_addr);
    p->killed = 1;
    return;
  }

  if (*pte & PTE_W)
  {
    cprintf("pid %d %s: pagefault: already writable.\n", p->pid, p->name);
    p->killed = 1;
    return;
  }

  if (!(*pte & PTE_COW))
  {
    cprintf("pid %d %s: pagefault: no cow flag\n", p->pid, p->name);
    p->killed = 1;
    return;
  }

  uint page_addr = PTE_ADDR(*pte);
  uint cow_reference_count = get_count(page_addr);
  uint flags = PTE_FLAGS(*pte);

  if (cow_reference_count > 1)
  {
    //allocate memory to new page
    //kalloc allocate one 4096-byte page of physical memory.
    // Returns a pointer that the kernel can use.
    // Returns 0 if the memory cannot be allocated.
    if ((new_mem = kalloc()) == 0)
    {
      cprintf("pid %d %s: pagefault: can't allocate memory\n", p->pid, p->name);
      p->killed = 1;
      return;
    }

    //cprintf("kalloc\n");
    //copy page to new memory
    memmove(new_mem, P2V(page_addr), PGSIZE);

    //change the page table entry to the new page
    *pte = V2P(new_mem) | flags | PTE_P | PTE_U | PTE_W;
    // When changing a valid page table entry to another
    // valid page table entry, you may need to clear the TLB (as in the lcr3())

    decrement_count(page_addr);
  }
  else if (cow_reference_count == 1)
  {

    //remove read only
    *pte |= PTE_W;
    //remove cow flag
    *pte &= ~PTE_COW;
  }
  else
  {
    panic("pagefault: reference count");
  }
  lcr3(V2P(p->pgdir));
}

//PAGEBREAK!
// Blank page.
//PAGEBREAK!
// Blank page.
//PAGEBREAK!
// Blank page.
