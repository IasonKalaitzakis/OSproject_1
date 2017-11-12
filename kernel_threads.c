
#include "tinyos.h"
#include "kernel_sched.h"
#include "kernel_proc.h"
#include "kernel_cc.h"

/** 
  @brief Create a new thread in the current process.
  */

void starting_threads() {
    int exitVal;
    Task task = CURTHREAD->owner_ptcb->task;
    int argl = CURTHREAD->owner_ptcb->argl;
    void* args = CURTHREAD->owner_ptcb->args;

    exitVal = task(argl,args);
    ThreadExit(exitVal);
}


void initialize_PTCB(PTCB* ptcb) 
{
  ptcb->argl = 0;
  ptcb->args = NULL;
  ptcb->refcount = 1;

  rlnode_init(& ptcb->ptcb_node, ptcb);
  ptcb->thread_exit = COND_INIT;
  ptcb->state = INIT;
}

PTCB* acquire_PTCB() 
{
  PTCB* ptr = (PTCB*)malloc(sizeof(PTCB));
  if (ptr==NULL)
  {
    fprintf(stderr, "Out of memory");
  } else {
    initialize_PTCB(ptr);
  }
  return ptr;
}

void free_PTCB(PTCB* ptcb) 
{
  TCB* tcb = ptcb->tcb;
  tcb->owner_pcb->referenceCounting--;
  rlist_remove(&ptcb->ptcb_node);
  free(ptcb);
}

void release_PTCB(PTCB* ptcb) 
{
  if (ptcb->refcount == 0){
    free_PTCB(ptcb);
  }
}

Tid_t sys_CreateThread(Task task, int argl, void* args)
{
  PCB* curproc = CURPROC;
  PTCB* curproc_ptcb;
  curproc_ptcb = acquire_PTCB();

  //if (curproc_ptcb == NULL) goto finish;
  curproc_ptcb->task = task;
  curproc_ptcb->argl = argl;
  curproc_ptcb->args = args;
  /*if (args!=NULL) {
    curproc_ptcb->args = malloc(argl);
    memcpy(curproc_ptcb->args, args, argl);
  }
  else
    curproc_ptcb->args = NULL;*/
  //rlnode_init(&newproc_PTCB->ptcb_node, newproc_PTCB);

  rlist_push_back(& curproc->list_of_PTCBS, & curproc_ptcb->ptcb_node);
  curproc->referenceCounting++;

  if(task!=NULL) {
    curproc_ptcb->tcb  = spawn_thread(curproc,starting_threads);
    curproc_ptcb->tcb->owner_ptcb = curproc_ptcb;
    wakeup(curproc_ptcb->tcb);
  } 

  if (curproc_ptcb!=NULL) {
  return (Tid_t) curproc_ptcb->tcb; /** Return TCB of new Thread*/
  } else {
    return NOPROC;
  }
}

/**
  @brief Return the Tid of the current thread.
 */
Tid_t sys_ThreadSelf()
{
  return (Tid_t) CURTHREAD;
}

/**
  @brief Join the given thread.
  */
int sys_ThreadJoin(Tid_t tid, int* exitval)
{
  int join_result;
  PCB* curproc = CURPROC; 
  //PTCB* curproc_ptcb;
  //curproc_ptcb = acquire_PTCB;  
  if (tid == NOPROC) {
    fprintf(stderr,"ERROR HERE");
  }

  TCB* childthread = (TCB*) tid;
  TCB* curthread = CURTHREAD;
  Thread_state curstate = childthread->state;
  PTCB* child_ptcb = childthread->owner_ptcb;

  /*Legallity Checks*/
  if (curthread == childthread){
    return -1;

  }

  if ((childthread->owner_pcb != curthread->owner_pcb)  || child_ptcb == NULL){
  //if (rlist_find(&curproc->list_of_PTCBS,childthread,NULL)  || child_ptcb == NULL){
    return -1;
  }

  if (childthread->state == EXITED || childthread->state == DETACHED){
    return -1; 
  }


  
  child_ptcb->refcount++;

  //kernel_unlock(); 
  join_result = kernel_wait(&child_ptcb->thread_exit, SCHED_USER);
  //kernel_lock();

  if (join_result == 0) {
    return -1;
  } 

  child_ptcb->refcount--;
  
  if(exitval != NULL){
  *exitval =child_ptcb->exitVal;  
  }

  release_PTCB(child_ptcb);
  

 // waitpid();

  return 0;
}

/**
  @brief Detach the given thread.
  */
int sys_ThreadDetach(Tid_t tid)
{
  PCB* curproc = CURPROC;
  TCB* currthread = (TCB*) tid;
  rlnode* thread_node = rlist_find(&curproc->list_of_PTCBS,currthread,NULL);
  if (thread_node == NULL || currthread->state == EXITED) {
    return -1; /**Error while detaching the thread*/
  }

  PTCB* ptcb_of_thread = currthread->owner_ptcb;
  Cond_Broadcast(&ptcb_of_thread->thread_exit); /**Wake up the threads which were joined with the current thread*/
  ptcb_of_thread->refcount = 1;  
  ptcb_of_thread->state = DETACHED;
  currthread->state = DETACHED;
  return 0; /**The thread has been detached*/
}

/**
  @brief Terminate the current thread.
  */
void sys_ThreadExit(int exitval)
{
  TCB* curthread = CURTHREAD;
  PTCB* curptcb = curthread->owner_ptcb;
  PCB* curpcb = curthread->owner_pcb;

  if (curthread != curthread->owner_pcb->main_thread) {
      curptcb->exitVal = exitval;

      while(curptcb->refcount == 1)
      {
        kernel_unlock();
        yield(SCHED_QUANTUM);
        kernel_lock();
      }

      kernel_broadcast(& curptcb->thread_exit); 
      curptcb->state = EXITED;
      release_PTCB(curptcb);
      kernel_sleep(EXITED,SCHED_USER);
  } else {
    if (curthread == curthread->owner_pcb->main_thread) {
      while(curpcb->referenceCounting==1) {
        PTCB* ptcb_next = curpcb->list_of_PTCBS.next->ptcb;
        sys_ThreadJoin((Tid_t)ptcb_next->tcb,NULL);
      }
      curptcb->exitVal = exitval;
      curptcb->state = EXITED;
      curptcb->refcount--;
      release_PTCB(curptcb);
      sys_Exit(exitval);
    }
  }
  //if (is_rlist_empty(& curpcb->list_of_PTCBS)) {
    //release_PCB(curpcb);
  //}
}
