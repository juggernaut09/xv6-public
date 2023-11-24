#include "fcntl.h"
#include "types.h"
#include "stat.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

int scheduler_type = 0;

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;
  p->priority = 60;
  p->num_run = 0;

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  // Initialize the values creation_time, end_time and total_time
  p->creation_time = ticks;
  p->end_time = 0;
  p->total_time = 0;
  p->rtime = 0;
  p->iotime = 0;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){ // changed
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;
  np->stack_tracker = curproc->stack_tracker; // changed
  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Intialize the end_time.
  curproc->end_time = ticks;
  curproc->total_time = curproc->end_time - curproc->creation_time;

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.

void
scheduler(void)
{
	struct cpu *c = mycpu();
	c->proc = 0;
	
	for(;;){
		// Enable interrupts on this processor.
		sti();

		acquire(&ptable.lock);
    if(scheduler_type == 0){ // DEFAULT round-robin
      struct proc *p = 0;		
			// Loop over process table looking for process to run.
			for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
				if(p->state != RUNNABLE)
					continue;
				// Switch to chosen process.  It is the process's job
				// to release ptable.lock and then reacquire it
				// before jumping back to us.
				c->proc = p;
				switchuvm(p);
				p->state = RUNNING;
				// cprintf("cpu %d, pname %s, pid %d, rtime %d\n", c->apicid, p->name, p->pid, p->rtime);
				swtch(&(c->scheduler), p->context);
				switchkvm();

				// Process is done running for now.
				// It should have changed its p->state before coming back.
				c->proc = 0;
			}
    }
		else if(scheduler_type == 1) { // FCFS
      struct proc *minP = 0, *p = 0;
			// Loop over process table looking for process to run.
			for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
				if(p->state != RUNNABLE)
					continue;

				// ignore init and sh processes from FCFS
				if(minP != 0){
					// here I find the process with the lowest creation time (the first one that was created)
					if(p->creation_time < minP->creation_time)
						minP = p;
				}
				else
					minP = p;
			}  

			if(minP != 0){
				// Switch to chosen process.  It is the process's job
				// to release ptable.lock and then reacquire it
				// before jumping back to us.
				c->proc = minP;
				switchuvm(minP);
				minP->state = RUNNING;
				// cprintf("cpu %d, pname %s, pid %d, rtime %d\n", c->apicid, minP->name, minP->pid, minP->rtime);
				swtch(&(c->scheduler), minP->context);
				switchkvm();

				// Process is done running for now.
				// It should have changed its p->state before coming back.
				c->proc = 0;
			}
    }
		else if(scheduler_type == 2) // PBS
    {
      struct proc *lowP = 0, *p = 0;
			for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
				if(p->state != RUNNABLE)
					continue;
				if(lowP == 0)
					lowP = p;
				else{
					if(lowP->priority > p->priority)
						lowP = p;
					else if(lowP->priority == p->priority && lowP->num_run > p->num_run)
						lowP = p;
				}
			}
			for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
				if(p->state != RUNNABLE) continue;
				if(p->priority == lowP->priority && p->num_run == lowP->num_run){
					lowP = p;
					break;
				}
			}
			if(lowP != 0){
				c->proc = lowP;
				switchuvm(lowP);
				lowP->state = RUNNING;
				lowP->num_run++;
				// cprintf("cpu %d, pname %s, pid %d, priority %d, rtime %d\n", c->apicid, lowP->name, lowP->pid, lowP->priority, lowP->rtime);
				swtch(&(c->scheduler), lowP->context);
				switchkvm();

				// Process is done running for now.
				// It should have changed its p->state before coming back.
				c->proc = 0;					
			}
    }
		release(&ptable.lock);
	}
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
// void
// yield(void)
// {
//   acquire(&ptable.lock);  //DOC: yieldlock
//   myproc()->state = RUNNABLE;
//   sched();
//   release(&ptable.lock);
// }
void
yield(void)
{
	acquire(&ptable.lock);  //DOC: yieldlock
  if(scheduler_type == 2){ //PBS
    struct proc *p = 0, *lowP = 0;
		for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
			if(p->state != RUNNABLE)
				continue;
			if(lowP == 0)
				lowP = p;
			else{
				if(p->priority <= lowP->priority)
					lowP = p;
			}
		}
		if(lowP != 0 && lowP->priority <= myproc()->priority){
			myproc()->state = RUNNABLE;
			sched();		
		}
  }
  else if(scheduler_type == 0) // DEFUALT round-robin
  { 
    myproc()->state = RUNNABLE;
		sched();
  }
	release(&ptable.lock);
}
// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s", p->pid, state, p->name);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

void uniqbasic(char **arr, int length){
    char buffer[100];
    for(int i = 0; i < length; i++){
        strncpy(buffer, arr[i], sizeof(buffer));  // Copying the strings to the buffer for the comparison
        if(strncmp(buffer, arr[i+1], sizeof(buffer)) == 0){
            continue;  // If the consective lines are same, continue with the next strings
        }else{
            cprintf("%s\n", buffer);
        }
    }
}

void uniqdfunction(char **arr, int length){
    char buffer[100];
    int flag = 0;
    for(int i = 0; i < length; i++){
        strncpy(buffer, arr[i], sizeof(buffer));
        if(strncmp(buffer, arr[i+1], sizeof(buffer)) == 0){
            flag = 1; // using flag to note that two lines are same
            continue;
        }else{
            if(flag == 1)
            cprintf("%s\n", buffer);
            flag = 0;
        }
    }
}

// keeping a count of the entered strings
void uniqcfunction(char **arr, int length){
    char buffer[100];
    int count = 1; 
    for(int i = 0; i < length; i++){
        strncpy(buffer, arr[i], sizeof(buffer));
        if(strncmp(buffer, arr[i+1], sizeof(buffer)) == 0){
            count++; // If both the lines are same, we increment the count  
            continue;
        }else{
            cprintf("%d %s\n",count, buffer); 
            count = 1;
        }
    }
}

// ignoring the case sensitivity 
void uniqifunction(char **arr, int length){
    char buffer[100];
    char temp[100];
    char next[100];
    int i=0;
    for(i = 0; i < length; i++){
        strncpy(buffer, arr[i], sizeof(buffer));
        strncpy(temp, buffer, sizeof(temp));
        strncpy(next, arr[i + 1], sizeof(next));

        // converting all the characters in the string to the lower case 
        for(int j=0;buffer[j]!='\0';j++){
            if(buffer[j]>=65 && buffer[j]<=90){
                buffer[j]=buffer[j]+32;
            }
        }

        for(int j=0;next[j]!='\0';j++){
            if(next[j]>=65 && next[j]<=90){
                next[j]=next[j]+32;
            }
        }

        // comparing both the cases and printing only the unique string
        if(strncmp(buffer, next, sizeof(buffer)) == 0){
            continue;
        }else{
            cprintf("%s\n", temp);
        }
    }
}


int uniq(char* flag, char **arr_of_strs, int length)
{
  cprintf("Uniq command is getting executed in kernel mode.\n");
  if(strncmp(flag, "basic", sizeof(flag)) == 0){
    uniqbasic(arr_of_strs, length);
  }
  if(strncmp(flag, "-d", sizeof(flag)) == 0){
    uniqdfunction(arr_of_strs, length);
  }
  if(strncmp(flag, "-c", sizeof(flag)) == 0){
    uniqcfunction(arr_of_strs, length);
  }
  if(strncmp(flag, "-i", sizeof(flag)) == 0){
    uniqifunction(arr_of_strs, length);
  }

  return -1;
}

int head(char **arr_of_strs, int len, int n)
{
  if(len < n){
      for(int i=0; i< len; i++){
          cprintf("%s\n", arr_of_strs[i]);
      }
  } else {
      if(len >= n){
          for(int i=0; i < n; i++){
              cprintf("%s\n", arr_of_strs[i]);
          }
      }
  }
  return -1;
}

int getprocstats(int *creation_time, int *end_time, int *total_time, int *wtime, int *rtime)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        *creation_time = p->creation_time;
        *end_time = p->end_time;
        *total_time = p->total_time;
        *wtime = p->end_time - p->creation_time - p->rtime;
				*rtime = p->rtime;
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

int ps(void)
{
  struct proc *p;
  //Enables interrupts on this processor.
  sti();

  //Loop over process table looking for process with pid.
  acquire(&ptable.lock);
  cprintf("name\tpid\tstate\t\tcreation time\tend time\ttotal time\n");
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == SLEEPING)
      cprintf("%s \t %d \t SLEEPING \t %d ms\t\t %d ms \t\t %d ms\n", p->name,p->pid, p->creation_time, p->end_time, p->total_time);
    else if(p->state == RUNNING)
      cprintf("%s \t %d \t RUNNING \t %d ms\t\t %d ms \t\t %d ms\n", p->name,p->pid, p->creation_time, p->end_time, p->total_time);
    else if(p->state == RUNNABLE)
      cprintf("%s \t %d \t RUNNABLE \t %d ms\t\t %d ms \t\t %d ms\n", p->name,p->pid, p->creation_time, p->end_time, p->total_time);
  }
  release(&ptable.lock);
  return 22;
}

int ps_pid(int pid)
{
  int found = 0;
  struct proc *p;
  //Enables interrupts on this processor.
  sti();

  //Loop over process table looking for process with pid.
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      found = 1;
      cprintf("name\tpid\tstate\t\tcreation time\tend time\ttotal time\n");
      if(p->state == SLEEPING)
        cprintf("%s \t %d \t SLEEPING \t %d ms\t\t %d ms \t\t %d ms\n", p->name,p->pid, p->creation_time, p->end_time, p->total_time);
      else if(p->state == RUNNING)
        cprintf("%s \t %d \t RUNNING \t %d ms\t\t %d ms \t\t %d ms\n", p->name,p->pid, p->creation_time, p->end_time, p->total_time);
      else if(p->state == RUNNABLE)
        cprintf("%s \t %d \t RUNNABLE \t %d ms\t\t %d ms \t\t %d ms\n", p->name,p->pid, p->creation_time, p->end_time, p->total_time);
      break;
    }
  }
  release(&ptable.lock);
  if(!found)
  {
    cprintf("ps: Process not found with the pid : %d.\n", pid);
    return -1;
  } 
  return 22;
}

int ps_pname(char *pname)
{
  struct proc *p;
  int found = 0;
  //Enables interrupts on this processor.
  sti();
  
  //Loop over process table looking for process with pid.
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(strncmp(pname, p->name, sizeof(pname)) == 0){
      found = 1;
      cprintf("name\tpid\tstate\t\tcreation time\tend time\ttotal time\n");
      if(p->state == SLEEPING)
        cprintf("%s \t %d \t SLEEPING \t %d ms\t\t %d ms \t\t %d ms\n", p->name,p->pid, p->creation_time, p->end_time, p->total_time);
      else if(p->state == RUNNING)
        cprintf("%s \t %d \t RUNNING \t %d ms\t\t %d ms \t\t %d ms\n", p->name,p->pid, p->creation_time, p->end_time, p->total_time);
      else if(p->state == RUNNABLE)
        cprintf("%s \t %d \t RUNNABLE \t %d ms\t\t %d ms \t\t %d ms\n", p->name,p->pid, p->creation_time, p->end_time, p->total_time);
      break;
    }
  }
  release(&ptable.lock);
  if(!found)
  {
    cprintf("ps: Process not found with the name : %s.\n", pname);
    return -1;
  } 
  return 22;
}

int set_scheduler(int sched)
{
  if(sched != 0 && sched != 1 && sched != 2) return -1;
  scheduler_type = sched;
  return 29;
}

// Changes Process priority
int set_priority(int pid, int priority){
  struct proc *p = 0;
  int old_priority = -1;
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      old_priority = p->priority;
      p->priority = priority;
      if(old_priority != priority)
        p->num_run = 0;
      break;
    }
  }
  release(&ptable.lock);
  return old_priority;
}

void update_process_time(){
	acquire(&ptable.lock);
	struct proc *p;
	for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
		if(p->state == RUNNING)
			p->rtime++;
		else if(p->state == SLEEPING)
			p->iotime++;
	}
	release(&ptable.lock);
}
