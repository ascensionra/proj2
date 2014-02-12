#include <stdio.h>
#include <stdlib.h>
#include "threads.h"

#define stack_size 4096
#define debug 0

void f3(void *arg);
void f2(void *arg);
void f1(void *arg);
struct thread *current = NULL;

typedef struct thread{
	int id;									//id for debugging
	void (*f)(void *arg);							//function pointer
	void* arg;								//function's argument
	char* stack;								//thread's stack
	struct thread* nextThread;						//pointer to next thread in queue
	struct thread* lastThread;						//pointer to previous thread
	char* base;								//stack base pointer
} thread;

//creates a new thread and initializes values
thread *thread_create(void (*f)(void *arg), void *arg, int id){			//remove id before submitting!!
	thread* newThread = malloc(sizeof(thread));				//new thread	
	newThread->id = id;							//thread id
	newThread->stack = malloc(stack_size);					//new stack
	newThread->base = newThread->stack;					//set base pointer
	printf("address = %p\n", (void*)newThread->stack);
	printf("mod = %d\n", (int)newThread->stack % 8);
	while((long)newThread->stack % 8 != 0){					//correct?
		newThread->stack++;
		printf("address = %p\n", (void*)newThread->stack);
	}
	if (debug){
	  printf("f = %p\n", f);
	}
	newThread->f = f;							//function pointer
	printf("newThread->f = %p\n", newThread->f);
	newThread->arg = arg;							//void* arg
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
	if(current == NULL){							//current thread = newThread, otherwise...
		current = newThread;
		current->nextThread = newThread;				//point to itself if only one in queue
		current->lastThread = newThread;				//^^
		printf("current's next thread = %p\n", current->nextThread);
	}
	//else, not empty queue
	else{
		current->lastThread->nextThread = newThread;			//last thread needs to point to a new last thread
		newThread->lastThread = current->lastThread;	//new thread's previous thread points to the previous last thread
		newThread->nextThread = current;							
		current->lastThread = newThread;
	}
	printf("current thread = %p\n\n", current);
	return;
}

//saves the current thread's context to its struct
void thread_yield(void){
	if(debug){
		printf("current thread = %p\n", current);
		printf("current's stack pointer = %p\n", current->stack);
		printf("current's base pointer = %p\n", current->base);
	}
	//save context to current thread (copies esp to current thread's stack pointer, and bsp to current thread's base pointer)
	__asm __volatile("mov %%rsp, %%rax" : "=a" (current->stack) : );
	__asm __volatile("mov %%rbp, %%rax" : "=a" (current->base) : );
	//move next thread's context into cpu
	__asm __volatile("mov %%rax, %%rsp" : : "a" (current->nextThread->stack) );
	__asm __volatile("mov %%rax, %%rbp" : : "a" (current->nextThread->base) );
	
	current = current->nextThread;
	if(debug){
		printf("current's stack pointer = %p\n", current->stack);
		printf("current's base pointer = %p\n", current->base);
		printf("current thread = %p\n", current);
	}
	return;
}

//picks next thread to execute
void schedule(void){

}

//
void dispatch(void){

}

//starts the threading process
void thread_start_threading(void){
	
}


void thread_exit(void){
	if(debug){
		printf("current thread = %p\n", current);
		printf("last thread id = %d\n", current->lastThread->id);
		printf("next thread id = %d\n", current->nextThread->id);
	}
	thread* oldThread = current;
	current->lastThread->nextThread = current->nextThread;			//removing from ring
	current->nextThread->lastThread = current->lastThread;
	current = current->nextThread;						//move to next thread
	free(oldThread->stack);										
	free(oldThread);		
	dispatch();
}

//prints the ring of threads
void printRing(int revs, int numThreads){
	int i = numThreads * revs;
	while(i > 0){
		printf("thread id = %d\n", current->id);
		current = current->nextThread;
		i--;
	}
}

int main(int argc, char **argv)
{
	int i = 0;
    struct thread *t1 = thread_create(f1, NULL, 1);
	thread* t2 = thread_create(f1, NULL, 2);
	thread* t3 = thread_create(f1, NULL, 3);
	thread* t4 = thread_create(f1, NULL, 4);
	thread* t5 = thread_create(f1, NULL, 5);
    thread_add_runqueue(t1);
	i++;
	thread_add_runqueue(t2);
	i++;
	thread_add_runqueue(t3);
	i++;
	thread_add_runqueue(t4);
	i++;
	thread_add_runqueue(t5);
	i++;
	printRing(3, i);
	thread_exit();
	i--;
	thread_exit();
	i--;
	t1 = thread_create(f1, NULL, 1);
	thread_add_runqueue(t1);
	i++;
	printRing(3, i);
	
	
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
