#include "thread_pool.h"

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
