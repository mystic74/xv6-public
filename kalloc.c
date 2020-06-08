// Physical memory allocator, intended to allocate
// memory for user processes, kernel stacks, page table pages,
// and pipe buffers. Allocates 4096-byte pages.

#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "spinlock.h"

void freerange(void *vstart, void *vend);
extern char end[]; // first address after kernel loaded from ELF file
                   // defined by the kernel linker script in kernel.ld

struct run
{
  struct run *next;
};

struct
{
  struct spinlock lock;
  int use_lock;
  struct run *freelist;
  uint num_free_pages; //for debug
  uint cow_reference_count[PHYSTOP / PGSIZE];
  //use the physical address of the page divided
  //by PGSIZE (that is, the physical page number)
  //to access the appropriate element of the array
  //PHYSTOP is the maximum physical memory address xv6 supports
} kmem;

static int freePages = 0;

int getFreePages()
{
  return freePages;
}

int getTotalPages()
{
  return PGROUNDDOWN(PHYSTOP - V2P(end)) / PGSIZE;
}
// Initialization happens in two phases.
// 1. main() calls kinit1() while still using entrypgdir to place just
// the pages mapped by entrypgdir on free list.
// 2. main() calls kinit2() with the rest of the physical pages
// after installing a full page table that maps them on all cores.
void kinit1(void *vstart, void *vend)
{
  initlock(&kmem.lock, "kmem");
  kmem.use_lock = 0;
  kmem.num_free_pages = 0;
  freerange(vstart, vend);
}

void kinit2(void *vstart, void *vend)
{
  freerange(vstart, vend);
  kmem.use_lock = 1;
}

void freerange(void *vstart, void *vend)
{
  char *p;
  p = (char *)PGROUNDUP((uint)vstart);
  for (; p + PGSIZE <= (char *)vend; p += PGSIZE)
  {
    kmem.cow_reference_count[V2P(p) / PGSIZE] = 0;
    kfree(p);
  }
}
//PAGEBREAK: 21
// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void kfree(char *v)
{
  struct run *r;

  if ((uint)v % PGSIZE || v < end || V2P(v) >= PHYSTOP)
    panic("kfree");

  if (kmem.use_lock)
    acquire(&kmem.lock);

  r = (struct run *)v;

  if (kmem.cow_reference_count[V2P(v) / PGSIZE] > 1)
  {
    kmem.cow_reference_count[V2P(v) / PGSIZE]--; //decrement count
  }
  else
  {
    // Fill with junk to catch dangling refs.
    memset(v, 1, PGSIZE);

    kmem.num_free_pages++;
    kmem.cow_reference_count[V2P(v) / PGSIZE] = 0;
    r->next = kmem.freelist;
    kmem.freelist = r;
  }

  if (kmem.use_lock)
    release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
char *
kalloc(void)
{
  struct run *r;

  if (kmem.use_lock)
    acquire(&kmem.lock);
  r = kmem.freelist;

  if (r)
  {
    if (kmem.cow_reference_count[V2P((char *)r) / PGSIZE] == 0)
    {
      kmem.freelist = r->next;
      kmem.num_free_pages--;
      kmem.cow_reference_count[V2P((char *)r) / PGSIZE] = 1;
    }
  }

  if (kmem.use_lock)
    release(&kmem.lock);
  return (char *)r;
}

uint get_num_of_free_pages(void)
{
  if (kmem.use_lock)
    acquire(&kmem.lock);
  uint free = kmem.num_free_pages;
  if (kmem.use_lock)
    release(&kmem.lock);
  return free;
}

void increment_count(uint page_add)
{

  if (kmem.use_lock)
    acquire(&kmem.lock);
  //cprintf("inc: old count for %d is %d\n", page_add, kmem.cow_reference_count[page_add/PGSIZE]);
  kmem.cow_reference_count[page_add / PGSIZE]++;
  //cprintf("inc: new count for %d is %d\n", page_add, kmem.cow_reference_count[page_add/PGSIZE]);
  if (kmem.use_lock)
    release(&kmem.lock);
}

void decrement_count(uint page_add)
{
  if (kmem.use_lock)
    acquire(&kmem.lock);
  //cprintf("dec: old count for %d is %d\n", page_add, kmem.cow_reference_count[page_add/PGSIZE]);
  kmem.cow_reference_count[page_add / PGSIZE]--;
  //cprintf("dec: new count for %d is %d\n", page_add, kmem.cow_reference_count[page_add/PGSIZE]);
  if (kmem.use_lock)
    release(&kmem.lock);
}

unsigned char get_count(uint page_add)
{
  if (kmem.use_lock)
    acquire(&kmem.lock);
  unsigned char count = kmem.cow_reference_count[page_add / PGSIZE];
  if (kmem.use_lock)
    release(&kmem.lock);
  return count;
}
