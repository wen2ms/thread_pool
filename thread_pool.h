#ifndef THREAD_POOL_H
#define THREAD_POOL_H

typedef struct ThreadPool ThreadPool;

ThreadPool* create_thread_pool(int min_num, int max_num, int queue_capacity);

void* worker(void* arg); 

#endif  // THREAD_POOL_H