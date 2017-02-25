#ifndef HTTPSERVER_THREADPOOL_H
#define HTTPSERVER_THREADPOOL_H

#include <pthread.h>
#include <stdlib.h>
#include <signal.h>


typedef struct thread_worker_queue
{
    void *(*process)(void *arg);  /* 线程处理的任务 */
    void *arg;                    /* 任务接口参数 */
    struct thread_worker_queue *next;/* 下一个节点 */
};

typedef struct thread_pool
{
    pthread_mutex_t queue_lock;

    struct thread_worker_queue *head;
    struct thread_worker_queue *tail;
    pthread_t *threadid;
    int max_thread_num;
    int min_thread_num;
    int num; //实际创建的线程个数
    int queue_size; //工作队列当前大小
};


struct thread_pool* init_thread_pool(int max_thread_num,int min_thread_num);
void push_thread_worker(struct thread_pool *pool,void *(*process)(void *arg),void *arg);
int  destroy_thread_pool(struct thread_pool *pool);
#endif //HTTPSERVER_THREADPOOL_H
