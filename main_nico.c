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

void stub(void);
void f3(void *arg);
void f2(void *arg);
void f1(void *arg);
struct thread *current = NULL;

typedef struct thread{
	int id;									//id for debugging
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
thread *thread_create(void (*f)(void *arg), void *arg, int id){			//remove id before submitting!!
	thread* newThread = malloc(sizeof(thread));				//new thread	
	newThread->id = id;							//thread id
	newThread->firstRun = 0;					//0 for hasnt run yet
	//newThread->stack = malloc(stack_size);					//new stack
	//newThread->base = newThread->stack;					//set base pointer

	posix_memalign((void*) &newThread->base, 8, 4096);					//bsp
	//newThread->stack--;
	newThread->stack = newThread->base + 4096;							//esp
		
	printf("base pointer = %p\n", newThread->base);
	printf("stack pointer = %p\n", newThread->stack);
	/* while((uint64_t)newThread->stack % 8 != 0){					//correct?
		newThread->stack++;
		if (debug) {
			printf("\naddress = %p\n", newThread->stack);
		}
	} */

	if (debug){
	  printf("<in thread_create>\nf = %p\n", f);
	}
	newThread->f = f;							//function pointer
	printf("newThread->f = %p\n", newThread->f);
	newThread->arg = arg;							//void* arg
	if (debug) {
	  printf("address = %p\n", newThread->stack);
	  printf("mod = %lu\n", (uint64_t)newThread->stack % 8);
	}
	printf("newThread address = %p\n<leaving thread_create>\n", newThread);
	return newThread;
}

//adds a new thread to the runqueue
void thread_add_runqueue(thread* newThread){
	printf("\n<in thread_add_runqueue>\ncurrent thread = %p\nadding to queue...\n", current);
	printf("newThread id = %d\n", newThread->id);
	
	//if empty queue
	if(current == NULL){							//current thread = newThread, otherwise...
		current = newThread;
		current->nextThread = newThread;				//point to itself if only one in queue
		current->prevThread = newThread;				//^^
		printf("current's next thread = %p\n", current->nextThread);
	}
	//else, not empty queue
	else{
		current->prevThread->nextThread = newThread;			//last thread needs to point to a new last thread
		newThread->prevThread = current->prevThread;	//new thread's previous thread points to the previous last thread
		newThread->nextThread = current;							
		current->prevThread = newThread;
	}
	printf("current thread = %p\nnextThread = %p\n<leaving thread_add_runqueue>\n", current, newThread);
	return;
}

//saves the current thread's context to its struct
void thread_yield(void){
	if(debug){
		printf("\n<in thread_yield>\ncurrent thread = %p\n", current);
		printf("current's stack pointer = %p\n", current->stack);
		printf("current's base pointer = %p\n", current->base);
	}		
	printf("before printing result\n");
	//call setjmp() here. longjmp will resume here when it is called inside dispatch. use current->id as index into buffer array for now.
	//if direct invocation
	schedule();
	dispatch();
	printf("after jump...\n");
	printf("before return\n");
	if(debug){
		printf("\ncurrent's stack pointer = %p\n", current->stack);
		printf("current's base pointer = %p\n", current->base);
		printf("current thread = %p\n<leaving thread_yield>\n", current);
	}
	return;
}

//picks next thread to execute. (round robin)
void schedule(void){
	printf("<in schedule>\n");
	current = current->nextThread;
	printf("<leaving schedule>\n");
}

//
void dispatch(void){
	//need to switch contexts to next thread
	//call longjmp with the buffer associated with the next thread to switch to a different thread's context. (buffer inside buffers[] at top)
	uint64_t* base = 0;
	uint64_t* stack = 0;
	//if direct invocation
	if(current->firstRun == 0){
	//change context (assembly) and call the thread's function
	printf("manual context switch\n");
	printf("base = %p\n", base);
	__asm__ volatile("mov %%rsp, %%rax" : "=a" (stack) : );
	__asm__ volatile("mov %%rbp, %%rax" : "=a" (base) : );
	printf("cpu bsp = %p\n", base);
	printf("cpu esp = %p\n\n", stack);
	
	printf("<first context switch>\n");
	
	__asm__ volatile("mov %%rax, %%rsp" : : "a" (current->stack));
	__asm__ volatile("mov %%rax, %%rbp" : : "a" (current->base));
	printf("...\n");
	
	__asm__ volatile("mov %%rsp, %%rax" : "=a" (stack) : );
	__asm__ volatile("mov %%rbp, %%rax" : "=a" (base) : );
	printf("cpu bsp = %p\n", base);
	printf("cpu esp = %p\n\n", stack);

	current->firstRun = 1;
	current->f(current->arg);
	thread_exit();
	}
	//change context with longjmp
	else{
		printf("before jump...\n");
		printf("current thread id = %d\n", current->id);
		longjmp(current->buffer, 1);	//jump to previously saved context.
		printf("after jump...\n");
	}
	printf("returning to thread_yield from dispatch\n");
	//returns here after longjmp
}

//starts the threading process. returns to main once all threads are finished.
void thread_start_threading(void){
	//loop until all threads are done
	while(current != NULL){
		dispatch();
		schedule();
	}
	//this is only reached once all threads are done? returns to main from here
}


void thread_exit(void){
	if(debug){
		printf("\n<in thread_exit>\ncurrent thread = %p\n", current);
		printf("last thread id = %d\n", current->prevThread->id);
		printf("next thread id = %d\n<leaving thread_exit>\n", current->nextThread->id);
	}
	thread* oldThread = current;
	current->prevThread->nextThread = current->nextThread;			//removing from ring
	current->nextThread->prevThread = current->prevThread;
	current = current->nextThread;						//move to next thread
	free(oldThread->stack);										
	free(oldThread);		
	dispatch();
}

//prints the ring of threads
	void printRing(int revs, int numThreads){			//revs=times around the ring
	int i = numThreads * revs;
	if (debug) { printf("\n<in printRing>\n"); }
	while(i > 0){
		printf("thread id = %d\n", current->id);
		current = current->nextThread;
		i--;
	}
	if (debug) { printf("\n<leaving printRing>\n"); }
}

int main(int argc, char **argv)
{
	printf("size of void* = %d\n", sizeof(void*));
	int i = 0;

    thread *t1 = thread_create(f1, NULL, 1);
	thread *t2 = thread_create(f1, NULL, 2);
	thread *t3 = thread_create(f1, NULL, 3);
/*
	thread* t2 = thread_create(f1, NULL, 2);
	thread* t3 = thread_create(f1, NULL, 3);
	thread* t4 = thread_create(f1, NULL, 4);
	thread* t5 = thread_create(f1, NULL, 5);
*/    
	thread_add_runqueue(t1);
	i++;
	thread_add_runqueue(t2);
	thread_add_runqueue(t3);
	//i++;
/*
	thread_add_runqueue(t2);
	i++;
	thread_add_runqueue(t3);
	i++;
	thread_add_runqueue(t4);
	i++;
	thread_add_runqueue(t5);
	i++;
*/
	thread_start_threading();
	i--;
/*
	thread_exit();
	i--;

	printRing(1, i);
	
	thread_exit();
	i--;
	
	printRing(1, i);
	
    thread_start_threading();
    printf("\nexited\n"); 
*/
    return 0;
}


void f3(void *arg)
{
    int i;
    while (1) {
        printf("thread 3: %d\n", i++);
        thread_yield();
    }
}

void f2(void *arg)
{
    int i = 0;
    while(1) {
        printf("thread 2: %d\n",i++);
        if (i == 10) {
            i = 0;
        }
        thread_yield();
    }
}

void f1(void *arg)
{
    int i = 100;
	uint64_t* base = 0;
	uint64_t* stack = 0;
	printf("\n\n");
	/* printf("current id = %d\nnextThread id = %d\nnextnextThread id = %d\n", current->id, current->nextThread->id, current->nextThread->nextThread->id);
	printf("current = %p\nnextThread = %p\nnextnextThread = %p\n", current, current->nextThread, current->nextThread->nextThread); */
    while(1) {
		/* printf("\nbsp = %p\n", current->base);
		printf("esp = %p\n", current->stack);
		__asm__ volatile("mov %%rsp, %%rax" : "=a" (stack) : );
		__asm__ volatile("mov %%rbp, %%rax" : "=a" (base) : );
		printf("cpu bsp = %p\n", base);
		printf("cpu esp = %p\n\n", stack); */
        printf("thread 1: %d   id: %d\n", i++, current->id);
        if (i == 110) {
            i = 100;
        }
		stub();
    }
    printf("Hello\n");
} 

void stub(){
	printf("current thread id before setjmp = %d\n", current->id);
	int result = setjmp(current->buffer);
	if(!result){
		thread_yield();
	}
}
