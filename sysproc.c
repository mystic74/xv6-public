#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

/*int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}*/

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}


/*task 2.1.3 updating the process signal mask*/
uint
sys_sigprocmask (void)
{
  uint sigmask;
  if (argint(0,(int*)&sigmask)<0)
    return -1;
  struct proc *curproc = myproc();
  uint old_mask = curproc -> signal_mask;
  curproc -> signal_mask = sigmask;
  return old_mask;
}

int
sys_sigaction(void)
{
  int signum;
  const struct sigaction *act;
  struct sigaction *oldact;
  struct proc *curproc = myproc();
  if (argint (0,&signum)<0)
    return -1;
  if (argptr(0,(char**)&act,sizeof(act))<0)
    return -1;
  if (argptr(0,(char**)&oldact,sizeof(oldact))<0)
    return -1;
  if (signum > 31 || signum < 0||signum == 9/*SIGKILL*/ ||signum == 17/*SIGSTOP*/)
    return -1;
  curproc -> signals_handlers[signum] = &act;
  if (oldact != (void*)NULL)
    return oldact->sigmask;
  return 0;
}

int
sys_sigret (void)
{
  sigret();
  return 1;
}

int 
sys_kill (void)
{
  int pid, signum;
  if (argint(0,(int*)&pid)<0)
    return -1;
  if (argint(0,(int*)&signum)<0)
    return -1;
  return kill(pid,signum);
}