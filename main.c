#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "thread_pool.h"

void task_func(void* arg) {
    int* num = (int*)arg;

    printf("thread %p, number = %d\n", pthread_self(), *num);

    usleep(1000);
}

int main() {
    ThreadPool* thread_pool = thread_pool_create(3, 10, 100);

    for (int i = 0; i < 100; ++i) {
        int* num = (int*)malloc(sizeof(int));
        *num = i + 100;

        thread_pool_add_task(thread_pool, task_func, num);
    }

    sleep(30);

    thread_pool_destroy(thread_pool);

    return 0;
}