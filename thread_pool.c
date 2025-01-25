#include "thread_pool.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const int kMaxAppendNum = 2;

typedef struct Task {
    void (*function)(void* arg);
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

        pthread_create(&thread_pool->manager_id, NULL, manager, thread_pool);
        for (int i = 0; i < min_num; ++i) {
            pthread_create(&thread_pool->thread_ids, NULL, worker, thread_pool);
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

void* manager(void* arg) {
    ThreadPool* thread_pool = (ThreadPool*)arg;

    while (!thread_pool->shotdown) {
        sleep(3);

        pthread_mutex_lock(&thread_pool->mutex_pool);

        int queue_size = thread_pool->queue_size;
        int alive_num = thread_pool->alive_num;

        pthread_mutex_unlock(&thread_pool->mutex_pool);

        pthread_mutex_lock(&thread_pool->mutex_busy);

        int busy_num = thread_pool->busy_num;

        pthread_mutex_unlock(&thread_pool->mutex_busy);

        if (queue_size > alive_num && alive_num < thread_pool->max_num) {
            pthread_mutex_lock(&thread_pool->mutex_pool);

            int counter = 0;

            for (int i = 0; i < thread_pool->max_num && counter < kMaxAppendNum && thread_pool->alive_num < thread_pool->max_num;
                 ++i) {
                if (thread_pool->thread_ids[i] == 0) {
                    pthread_create(&thread_pool->thread_ids[i], NULL, worker, thread_pool);

                    counter++;
                    thread_pool->alive_num++;
                }
            }

            pthread_mutex_unlock(&thread_pool->mutex_pool);
        }

        if (busy_num * 2 < alive_num && alive_num > thread_pool->min_num) {
            pthread_mutex_lock(&thread_pool->mutex_pool);

            thread_pool->exit_num = kMaxAppendNum;

            pthread_mutex_unlock(&thread_pool->mutex_pool);

            for (int i = 0; i < kMaxAppendNum; ++i) {
                pthread_cond_signal(&thread_pool->is_full);
            }
        }
    }

    return NULL;
}

void* worker(void* arg) {
    ThreadPool* thread_pool = (ThreadPool*)arg;

    while (1) {
        phtread_mutex_lock(&thread_pool->mutex_pool);

        while (thread_pool->queue_size == 0 && !thread_pool->shotdown) {
            pthread_cond_wait(&thread_pool->is_full, &thread_pool->mutex_pool);

            if (thread_pool->exit_num > 0)  {
                thread_pool->exit_num--;
                pthread_mutex_unlock(&thread_pool->mutex_pool);

                pthread_exit(NULL);
            }
        }

        if (thread_pool->shotdown) {
            pthread_mutex_unlock(&thread_pool->mutex_pool);

            pthread_exit(NULL);
        }

        Task task;

        task.function = thread_pool->task_queue[thread_pool->queue_front].function;
        task.arg = thread_pool->task_queue[thread_pool->queue_front].arg;

        thread_pool->queue_front = (thread_pool->queue_front + 1) % thread_pool->queue_capacity;
        thread_pool->queue_size--;

        pthread_mutex_unlock(&thread_pool->mutex_pool);

        printf("thread %p starts working...\n", pthread_self());
        pthread_mutex_lock(&thread_pool->mutex_busy);

        thread_pool->busy_num++;

        pthread_mutex_unlock(&thread_pool->mutex_busy);

        task.function(task.arg);

        free(task.arg);
        task.arg = NULL;

        printf("thread %p ends working...\n", pthread_self());
        pthread_mutex_lock(&thread_pool->mutex_busy);

        thread_pool->busy_num--;

        pthread_mutex_unlock(&thread_pool->mutex_busy);
    }

    return NULL;
}