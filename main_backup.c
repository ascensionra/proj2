#include <stdio.h>
#include <stdlib.h>
#include <setjmp.h>
#include <stdint.h>
#include "threads.h"

//#define MAX_THREADS 10
#define stack_size 4096
#define debug 0

//static jmp_buf buffers[MAX_THREADS];			//holds jum_buf's for use with setjmp and longjmp.
static jmp_buf mainBuf;							//buffer for main thread execution
static int mainFirstRun = 0;					//indicates first run of dispatch
static int numThreads = 0;

void f5(void *arg);
void f4(void *arg);
void f3(void *arg);
void f2(void *arg);
void f1(void *arg);
void print_queue(void);
struct thread *current = NULL;

typedef struct thread{
	int firstRun;							//0 if hasnt run yet
	jmp_buf buffer;							//holds context info for switching
	void (*f)(void *arg);					//function pointer
	void* arg;								//function's argument
	uint64_t* stack;							//thread's stack
	struct thread* nextThread;				//pointer to next thread in queue
	struct thread* prevThread;				//pointer to previous thread
	uint64_t* base;								//stack base pointer
} thread;

//creates a new thread and initializes values
thread *thread_create(void (*f)(void *arg), void *arg){			//remove id before submitting!!
	thread* newThread = malloc(sizeof(thread));				//new thread	
	newThread->firstRun = 0;					//0 for hasnt run yet
	posix_memalign((void*) &newThread->base, 8, 4096);
        newThread->stack = newThread->base + 4096;
/*
	while((uint64_t)newThread->stack % 8 != 0){					//correct?
		newThread->stack++;
		if (debug) {
			printf("\naddress = %p\n", newThread->stack);
		}
	}
*/
	if (debug){
	  printf("<in thread_create>\nf = %p\n", f);
	}
	newThread->f = f;							//function pointer
        if (debug) { printf("newThread->f = %p\n", newThread->f); }
	newThread->arg = arg;							//void* arg
	if (debug) {
	  printf("address = %p\n", newThread->stack);
	  printf("mod = %lu\n", (uint64_t)newThread->stack % 8);
	}
	if (debug) {
          printf("newThread address = %p\n<leaving thread_create>\n", newThread);
	}
	return newThread;
}

//adds a new thread to the runqueue
void thread_add_runqueue(thread* newThread){
	if (debug) { printf("\n<in thread_add_runqueue>\ncurrent thread = %p\nadding to queue...\n", current); }
	
	//if empty queue
	if(current == NULL){							//current thread = newThread, otherwise...
		current = newThread;
		current->nextThread = newThread;				//point to itself if only one in queue
		current->prevThread = newThread;				//^^
		if (debug) { printf("current's next thread = %p\n", current->nextThread); }
	}
	//else, not empty queue
	else{
		current->prevThread->nextThread = newThread;			//last thread needs to point to a new last thread
		newThread->prevThread = current->prevThread;	//new thread's previous thread points to the previous last thread
		newThread->nextThread = current;							
		current->prevThread = newThread;
	}
	if (debug) { printf("current thread = %p\n<leaving thread_add_runqueue>\n", current); }
	return;
}

//saves the current thread's context to its struct
void thread_yield(void){
	if(debug){
		printf("\n<in thread_yield>\n\tcurrent thread = %p\n", current);
		printf("\tcurrent's stack pointer = %p\n", current->stack);
		printf("\tcurrent's base pointer = %p\n", current->base);
		printf("\tcurrent's next = %p\n", current->nextThread);
		printf("\tcurrent's prev = %p\n", current->prevThread);
		printf("\tbefore printing result\n");
	}		
	//call setjmp() here. longjmp will resume here when it is called inside dispatch. use current->id as index into buffer array for now.
	//if direct invocation
        if (!setjmp(current->buffer)) {
	    if (debug) { printf("\t<thread_yield> calling schedule()\n"); }
	  schedule();
	    if (debug) { printf("\t<thread_yield> calling dispatch()\n"); }
	  dispatch();
	} 
/*	else {
	    if(debug){
		printf("<back in thread_yield>\n\tafter jump...\n");
		printf("\tbefore return\n");
		printf("\tcurrent's stack pointer = %p\n", current->stack);
		printf("\tcurrent's base pointer = %p\n", current->base);
		printf("\tcurrent thread = %p\n<leaving thread_yield>\n", current);
	    }
        
            if (debug) { printf("<thread_yield> calling longjmp\n"); }
	    longjmp(current->buffer,2);
	}
*/
}
//picks next thread to execute. (round robin)
void schedule(void){
	if (debug) { printf("\n<in schedule>\n"); }
//	if ( current == NULL ) { return; }
//	if ( current->nextThread != current ) {
          current = current->nextThread;
	  if (debug) { printf("current= %p\n", current); }
//	} 
/*        else {
	if (debug) { printf("\tno nextThread, current= %p\n", current); }
	}
*/	if (debug) { printf("<leaving schedule>\n"); }
}

void dispatch(void)
{
  if (debug) { printf("\n<dispatch>\ncurrent = %p\n",current); }
//  if(!setjmp(current->prevThread->buffer))
//  {
    if(current->firstRun == 0)
    {
      if (debug) { 
	printf("\t<dispatch first run block>\n"); 
	printf("\tmanual context switch\n");
      }
      __asm__ volatile("mov %%rax, %%rsp" :: "a" (current->stack));
      __asm__ volatile("mov %%rax, %%rbp" :: "a" (current->base));

      current->firstRun = 1;
      current->f(current->arg);
      if (debug) { printf("************* <dispatch> calling thread_exit\n"); }
      thread_exit();
    }
    else { 
      if (debug) { printf("\t<dispatch else block>\n\tbefore jump...\n"); }
      longjmp(current->buffer, 1);
      if (debug) { printf("\t<back in dispatch>\n\tafter jump\n"); }
    }
//  }
  if (debug) { printf("<leaving dispatch>\n"); }
}

//starts the threading process. returns to main once all threads are finished.
void thread_start_threading(void){
	//loop until all threads are done
/*
	while(current != NULL){
		schedule();
		dispatch();
	}
*/
        if (!setjmp(mainBuf)) {
		schedule();
		dispatch();
	}
        if (debug) { printf("\n<back in thread_start_threading> current = NULL\n"); } 
	//this is only reached once all threads are done? returns to main from here
}


void thread_exit(void){
	if(debug){
		printf("\n<in thread_exit>\ncurrent thread stack = %p\n", current->stack);
		printf("\tprev thread = %p\n", current->prevThread);
		printf("\tnext thread = %p\n", current->nextThread);
	}
//        print_queue();
	thread* oldThread = current;
	if ( current->nextThread == current ) 
	{ 
	  if (debug) { printf("<thread exit> last thread, killing it\n"); }
	  if (debug) { printf("\tfree(oldThread->base)\n"); }
	  free(oldThread->base);
	  if (debug) { printf("\tfree(oldThread)\n"); }
	  free(oldThread);
	  if (debug) { printf("\tset current = NULL\n"); }
          current = NULL;
	  //dispatch();
	  if (debug) { printf("\tlongjmp back to mainBuf\n"); }
	  longjmp(mainBuf,1);
	  if (debug) { printf("\t<back in thread_exit after dispatch>\n"); }
	}
	else {
	  if (debug) { printf("<thread_exit> just set oldThread=current\n"); }
	  current->prevThread->nextThread = current->nextThread;			//removing from ring
	  if (debug) { printf("<thread_exit> curr->prev->next = curr->next\n"); }
	  current->nextThread->prevThread = current->prevThread;
	  if (debug) { printf("<thread_exit> curr->next->prev = curr->prev\n"); }
	  current = current->nextThread;						//move to next thread
	  if (debug) { printf("<thread_exit> curr = curr->next\n"); }

          if (debug) { printf("oldThread->stack = %p\n", oldThread->stack);  }

	  free(oldThread->base);										
	  if (debug) { printf("<thread_exit> free(oldThread->stack\n"); }
	  free(oldThread);
	  if (debug) { printf("<leaving thread_exit> calling dispatch()\n"); }
 	  dispatch();
	}
}

//prints the ring of threads
	void printRing(int revs, int numThreads){			//revs=times around the ring
	int i = numThreads * revs;
	if (debug) { printf("\n<in printRing>\n"); }
	while(i > 0){
		printf("thread = %p\n", current);
		current = current->nextThread;
		i--;
	}
	if (debug) { printf("\n<leaving printRing>\n"); }
}

void print_queue(void)
{
  if (debug) { printf("\n<in print_queue>\n"); }
  thread *temp = current;
  int i = numThreads;
  while(temp && i > 0) {
    printf("thread = %p\n", temp);
    temp = temp->nextThread;
    i--;
  } 
  if (debug) { printf("<leaving print_queue>\n"); }
}

int main(int argc, char **argv)
{
	int i = 0;

  	thread *t1 = thread_create(f5, NULL);
//	thread *t2 = thread_create(f4, NULL);
/*
	thread* t3 = thread_create(f3, NULL);
	thread* t4 = thread_create(f1, NULL, 4);
	thread* t5 = thread_create(f1, NULL, 5);
*/    
	thread_add_runqueue(t1);
	i++;
	numThreads++;
/*
	thread_add_runqueue(t2);
	i++;
	numThreads++;

	thread_add_runqueue(t3);
	i++;
	numThreads++;
	thread_add_runqueue(t4);
	i++;
	numThreads++;
	thread_add_runqueue(t5);
	i++;
	numThreads++;

	//printRing(1, i);
	
	thread_start_threading();
	i--;

	thread_exit();
	i--;

	printRing(1, i);
	
	thread_exit();
	i--;
	
	printRing(1, i);
*/	
    thread_start_threading();
    printf("\nexited\n"); 

    return 0;
}


void f3(void *arg)
{
    int i;
    while (1) {
        printf("thread %p: %d\n",current, i++);
        thread_yield();
    }
}

void f2(void *arg)
{
    int i = 0;
    while(1) {
        printf("thread %p: %d\n",current, i++);
        if (i == 10) {
            i = 0;
        }
        thread_yield();
    }
}

void f1(void *arg)
{
    int i = 100;
     struct thread *t2 = thread_create(f2, NULL);
    thread_add_runqueue(t2);
    struct thread *t3 = thread_create(f3, NULL);
    thread_add_runqueue(t3); 
    while(1) {
        printf("thread %p: %d\n", current, i++);
        if (i == 110) {
            i = 100;
        }
	thread_yield();
    }
/*
    printf("Hello\n");
    thread_yield();
    printf(" World\n");
*/
} 

void f4(void *arg)
{
  printf("Hello\n");
  thread_yield();
  printf("World\n");
}
 void f5(void *arg)
{
  struct thread *t1 = thread_create(f1,NULL);
  thread_add_runqueue(t1);
  struct thread *t2 = thread_create(f4,NULL);
  thread_add_runqueue(t2);
}
