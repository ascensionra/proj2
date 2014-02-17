#include "threads.h"
#include <stdlib.h>
#include <stdio.h>

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
    }
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



int main(int argc, char **argv)
{
	/* //test1
  	struct thread *t1 = thread_create(f1, NULL);
    thread_add_runqueue(t1);
    thread_start_threading(); */
	
	//test2
	struct thread *t1 = thread_create(f1, NULL);
	struct thread *t2 = thread_create(f2, NULL);
	struct thread *t3 = thread_create(f3, NULL);
	thread_add_runqueue(t1);
	thread_add_runqueue(t2);
	thread_add_runqueue(t3);
	thread_start_threading();
	
    printf("\nexited\n");
    return 0;
}
