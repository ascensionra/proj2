#include <stdio.h>
#include "threads.h"

#define stack_size 4096
#define debug 0

void f3(void *arg);
void f2(void *arg);
void f1(void *arg);
struct thread *current = NULL;

typedef struct thread{
	void (*f)(void *arg);								//function pointer
	void* arg;											//function's argument
	char* stack;										//thread's stack
	struct thread* nextThread;									//pointer to next thread in queue
	char* base;											//stack base pointer
} thread;

//creates a new thread and initializes values
thread *thread_create(void (*f)(void *arg), void *arg){
	thread* newThread = malloc(sizeof(thread));			//new thread	
	newThread->stack = malloc(stack_size);				//new stack
	newThread->base = newThread->stack;					//set base pointer
	printf("address = %p\n", (void*)newThread->stack);
	printf("mod = %d\n", (int)newThread->stack % 8);
	while((long)newThread->stack % 8 != 0){				//correct?
		newThread->stack++;
		printf("address = %p\n", (void*)newThread->stack);
	}
	if (debug){
	  printf("f = %p\n", f);
	}
	newThread->f = f;									//function pointer
	printf("newThread->f = %p\n", newThread->f);
	newThread->arg = arg;								//void* arg
	if (debug) {
	  printf("address = %p\n", (void*)newThread->stack);
	  printf("mod = %d", (int)newThread->stack % 8);
	}
	printf("newThread address = %p\n", newThread);
	return newThread;
}

//adds a new thread to the runqueue
void thread_add_runqueue(thread* newThread){
	printf("current thread = %p\nadding to queue...\n", current);
	
	//if empty queue
	if(current == NULL){								//current thread = newThread, otherwise...
		current = newThread;
		current->nextThread = newThread;				//point to itself if only one in queue
		printf("current's next thread = %p\n", current->nextThread);
	}
	//else, not empty queue
	else{
		newThread->nextThread = current->nextThread;	//point newThread to current's next thread
		current->nextThread = newThread;				//current's next thread is the new thread
		//this always puts the new thread right after the current thread
	}
	printf("current thread = %p\n", current);
	return;
}

//saves the current thread's context to its struct
void thread_yield(void){
	//save cpu's base pointer and stack pointer to current thread
	//move next thread's base pointer and stack pointer to cpu
	printf("current thread = %p\n", current);
	//save context
	__asm __volatile("mov %%rsp, %%rax" : "=a" (current->stack) : );
	__asm __volatile("mov %%rbp, %%rax" : "=a" (current->base) : );
	//move new context in
	__asm __volatile("mov %%rax, %%rsp" : : "a" (current->nextThread->stack) );
	__asm __volatile("mov %%rax, %%rbp" : : "a" (current->nextThread->stack) );
	
	current = current->nextThread;
	printf("current thread = %p\n", current);
	return;
}

int main(int argc, char **argv)
{
    struct thread *t1 = thread_create(f1, NULL);
    thread_add_runqueue(t1);
	/*
    thread_start_threading();
    printf("\nexited\n"); */
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
    /* int i = 100;
    struct thread *t2 = thread_create(f2, NULL);
    thread_add_runqueue(t2);
    struct thread *t3 = thread_create(f3, NULL);
    thread_add_runqueue(t3);
    while(1) {
        printf("thread 1: %d\n", i++);
        if (i == 110) {
            i = 100;
        }
        thread_yield();
    } */
} 
