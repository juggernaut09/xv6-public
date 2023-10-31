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

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

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

int
 sys_uniq(void) {
    char *flag, *argv[MAXARG];
    uint uargv, uarg;
    int length;
    if(argstr(0, &flag) < 0 || argint(1, (int*)&uargv) < 0 || argint(2, &length) < 0){
      return -1;
    }
    memset(argv, 0, sizeof(argv));
    for(int i=0;; i++){
      if(i >= NELEM(argv))
        return -1;
      if(fetchint(uargv+4*i, (int*)&uarg) < 0)
        return -1;
      if(uarg == 0){
        argv[i] = 0;
        break;
      }
      if(fetchstr(uarg, &argv[i]) < 0)
        return -1;
    }
    return uniq(flag, argv, length);

 }

int sys_head(void) {
  char *argv[MAXARG];
  int len, n;
  uint uargv, uarg;
  if(argint(0, (int*)&uargv) < 0 || argint(1, &len) < 0 || argint(2, &n) < 0){
    return -1;
  }
  memset(argv, 0, sizeof(argv));
    for(int i=0;; i++){
      if(i >= NELEM(argv))
        return -1;
      if(fetchint(uargv+4*i, (int*)&uarg) < 0)
        return -1;
      if(uarg == 0){
        argv[i] = 0;
        break;
      }
      if(fetchstr(uarg, &argv[i]) < 0)
        return -1;
    }
  return head(argv, len, n);
}

int sys_head_message(void) {
  cprintf("Head command is getting executed in kernel mode.\n");
  return -1;
}

int sys_getprocstats(void) 
{
  int *creation_time;
  int *end_time;
  int *total_time;
  int *wtime;
  int *rtime;
  
  if(argptr(0, (char**)&creation_time, sizeof(int)) < 0)
    return 12;

  if(argptr(1, (char**)&end_time, sizeof(int)) < 0)
    return 13;
  
  if(argptr(2, (char**)&total_time, sizeof(int)) < 0)
    return 14;
  
  if(argptr(3, (char**)&wtime, sizeof(int)) < 0)
    return 15;
  
  if(argptr(4, (char**)&rtime, sizeof(int)) < 0)
    return 16;

  return getprocstats(creation_time, end_time, total_time, wtime, rtime);
}

int sys_ps(void)
{
  return ps();
}

int sys_ps_pid(void)
{
  int pid;
  if (argint(0, &pid) < 0 ) return -1;
  return ps_pid(pid);
}

int sys_ps_pname(void)
{
  char *pname;
  if(argstr(0, &pname) < 0) return -1;
  return ps_pname(pname);
}

int  sys_set_scheduler(void)
{
  int sched;
  if (argint(0, &sched) < 0 ) return -1;
  return set_scheduler(sched);
}

int
sys_set_priority(void){
  int pid, pr;
  if(argint(0, &pid) < 0)
    return -1;
  if(argint(1, &pr) < 0)
    return -1;
  return set_priority(pid, pr);
}