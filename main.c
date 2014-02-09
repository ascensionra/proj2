#include <stdio.h>
#include "threads.h"

#define stack_size 4096
#define debug 0

void f3(void *arg);
void f2(void *arg);
void f1(void *arg);
struct thread *current;

typedef struct thread{
	void (*f)(void *arg);								//function pointer
	void* arg;											//function's argument
	char* stack;										//thread's stack
	//thread pointer maybe??
	char* base;											//stack base pointer
} thread;

thread *thread_create(void (*f)(void *arg), void *arg){
	thread* newThread = malloc(sizeof(thread));			//new thread	
	newThread->stack = malloc(stack_size);				//new stack
	newThread->base = newThread->stack;					//set base pointer
	printf("address = %p\n", (void*)newThread->stack);
	printf("mod = %d\n", (int)newThread->stack % 8);
	//newThread->stack++;
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
	return newThread;
	
}

int main(int argc, char **argv)
{
    struct thread *t1 = thread_create(f1, NULL);
    /* thread_add_runqueue(t1);
    thread_start_threading();
    printf("\nexited\n"); */
    return 0;
}



void thread_yield(void){

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
