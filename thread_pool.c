#include "thread_pool.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>

typedef struct Task {
    void (* function)(void* arg);
    void* arg;
} Task;

struct ThreadPool {
    Task* task_queue;

    int queue_capacity;
    int queue_size;
    int queue_front;
    int queue_rear;

    pthread_t manager_id;
    pthread_t* thread_ids;

    int min_num;
    int max_num;
    int busy_num;
    int alive_num;
    int exit_num;

    pthread_mutex_t mutex_pool;
    pthread_mutex_t mutex_busy;

    pthread_cond_t is_full;
    pthread_cond_t is_empty;

    int shotdown;
};

ThreadPool* create_thread_pool(int min_num, int max_num, int queue_capacity) {
    ThreadPool* thread_pool = (ThreadPool*)malloc(sizeof(ThreadPool));

    do {
        if (thread_pool == NULL) {
            printf("malloc thread_pool failed...\n");
            break;
        }

        thread_pool->thread_ids = (pthread_t*)malloc(sizeof(pthread_t) * max_num);

        if (thread_pool->thread_ids == NULL) {
            printf("malloc thread_ids failed...\n");
            break;
        }

        memset(thread_pool->thread_ids, 0, sizeof(pthread_t) * max_num);

        thread_pool->min_num = min_num;
        thread_pool->max_num = max_num;
        thread_pool->busy_num = 0;
        thread_pool->alive_num = min_num;
        thread_pool->exit_num = 0;

        if (pthread_mutex_init(&thread_pool->mutex_pool, NULL) != 0 || ptrhead_mutex_init(&thread_pool->mutex_pool, NULL) != 0 ||
            pthread_cond_init(&thread_pool->is_full, NULL) != 0 || pthread_cond_init(&thread_pool->is_empty, NULL) != 0) {
            printf("mutex or condition init failed...\n");
            break;
        }

        thread_pool->task_queue = (Task*)malloc(sizeof(Task) * queue_capacity);

        if (thread_pool->task_queue == NULL) {
            printf("malloc task_queue failed...\n");
            break;
        }

        thread_pool->queue_capacity = queue_capacity;
        thread_pool->queue_size = 0;
        thread_pool->queue_front = 0;
        thread_pool->queue_rear = 0;

        thread_pool->shotdown = 0;

        pthread_create(&thread_pool->manager_id, NULL, manager, NULL);
        for (int i = 0; i < min_num; ++i) {
            pthread_create(&thread_pool->thread_ids, NULL, worker, NULL);
        }

        return thread_pool;
    } while (0);

    if (thread_pool && thread_pool->thread_ids) {
        free(thread_pool->thread_ids);
    }

    if (thread_pool && thread_pool->task_queue) {
        free(thread_pool->task_queue);
    }

    if (thread_pool) {
        free(thread_pool);
    }

    return NULL;
}
