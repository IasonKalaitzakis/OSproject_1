
#include "tinyos.h"
#include "kernel_sched.h"
#include "kernel_proc.h"

/** 
  @brief Create a new thread in the current process.
  */

void starting_threads() {
    int exitVal;
    Task task = CURPROC->main_task;
    int argl = CURPROC->argl;
    void* args = CURPROC->args;

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

Tid_t sys_CreateThread(Task task, int argl, void* args)
{
  PCB* curproc = CURPROC;
  PTCB* curproc_ptcb;
  curproc_ptcb = acquire_PTCB();

  //if (curproc_ptcb == NULL) goto finish;
  curproc_ptcb->task = task;
  curproc_ptcb->argl = argl;
  if (args!=NULL) {
    curproc_ptcb->args = malloc(argl);
    memcpy(curproc_ptcb->args, args, argl);
  }
  else
    curproc_ptcb->args = NULL;
  //rlnode_init(&newproc_PTCB->ptcb_node, newproc_PTCB);

  rlist_push_back(& curproc->list_of_PTCBS, & curproc_ptcb->ptcb_node);

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
	return -1;
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

}



void free_PTCB(PTCB* ptcb) 
{
  free(ptcb->tcb);
  free(ptcb);
}

void release_PTCB(PTCB* ptcb) 
{
  if (ptcb->refcount == 0){
    free_PTCB(ptcb);
  }
}

