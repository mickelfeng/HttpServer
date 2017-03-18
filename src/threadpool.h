#ifndef HTTPSERVER_THREADPOOL_H
#define HTTPSERVER_THREADPOOL_H

#include <pthread.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>

struct thread_worker_queue
{
    void *(*process)(void *arg);  //线程处理的任务
    void *arg;                    //任务参数
    struct thread_worker_queue *next;
};

struct thread_pool
{
    pthread_mutex_t queue_lock;
    pthread_cond_t queue_ready;//条件锁，当有任务加入时，唤醒等待线程

    struct thread_worker_queue *head;
    struct thread_worker_queue *tail;
    pthread_t *threadid;
    pthread_t keepalive_pid;
    int thread_num;
    int num; //实际创建的线程个数
    int queue_size; //工作队列当前大小
};

pthread_t keepalive_pid;

struct thread_pool *init_thread_pool(int thread_num);

void push_thread_worker(struct thread_pool *pool, void *(*process)(void *arg), void *arg);

int destroy_thread_pool(struct thread_pool *pool);

int create_detach_thread(struct thread_pool *pool, int index);

void thread_pool_keepalive(void *arg);

void *thread_routine(void *arg);

#endif //HTTPSERVER_THREADPOOL_H
