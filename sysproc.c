#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"

extern int current_sched_stg;

int sys_fork(void)
{
  return fork();
}

int sys_exit(void)
{
  int status;
  if (argint(0, &status) < 0)
    return -1;
  exit(status);
  return 0; // not reached
}

int sys_wait(void)
{
  int status;
  if (argint(0, &status) < 0)
    return -1;
  return wait((int *)status);
}

int sys_set_policy(void)
{
  int policy;
  if (argint(0, &policy) < 0)
  {
    return -1;
  }

  if ((policy >= number_of_polices) || (policy < 0))
  {
    return -1;
  }

  current_sched_stg = policy;

  return 0;
}

int sys_kill(void)
{
  int pid;

  if (argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int sys_getpid(void)
{
  return myproc()->pid;
}

//for task 4.2
int sys_set_ps_priority(void)
{
  int priority;

  if (argint(0, &priority) < 0)
    return -1;

  myproc()->ps_priority = priority;
  return 0;
}
//for task 4.3
int sys_set_cfs_priority(void)
{
  int priority;

  if (argint(0, &priority) < 0)
    return -1;

  if (priority > 3 || priority < 1)
    return 1;
  if (priority == 1)
  {
    myproc()->cfs_priority = 0.75;
  }
  if (priority == 2)
  {
    myproc()->cfs_priority = 1;
  }
  if (priority == 3)
  {
    myproc()->cfs_priority = 1.25;
  }
  return 0;
}

//for task 4.5
int sys_proc_info(void)
{
  struct perf performance;
  if (argptr(0, (void *)&performance, sizeof(performance)) < 0)
    return -1;

  performance.ps_priority = myproc()->ps_priority;
  performance.rtime = myproc()->rtime;
  performance.stime = myproc()->stime;
  performance.retime = myproc()->retime;
  return 0;
}

int sys_mysize(void)
{
  return myproc()->sz;
}

int sys_sbrk(void)
{
  int addr;
  int n;

  if (argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if (growproc(n) < 0)
    return -1;
  return addr;
}

int sys_sleep(void)
{
  int n;
  uint ticks0;

  if (argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n)
  {
    if (myproc()->killed)
    {
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
int sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}
