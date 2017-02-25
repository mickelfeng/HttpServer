#include "threadpool.h"

struct thread_pool * init_thread_pool(int max_thread_num,int min_thread_num)
{
    struct thread_pool *pool=(struct thread_pool *)calloc(1,sizeof(struct thread_pool));
    pool->max_thread_num=max_thread_num;
    pool->min_thread_num=min_thread_num;

    pool->head=(struct thread_worker_queue*)calloc(1,sizeof(struct thread_worker_queue));
    pool->tail=pool->head;
    pool->threadid=NULL;
    return pool;
}

void push_thread_worker(struct thread_pool *pool,void *(*process)(void *arg),void *arg)
{
    struct thread_worker_queue *p=(struct thread_worker_queue *)calloc(1,sizeof(struct thread_worker_queue));

    pthread_mutex_lock(&(pool->queue_lock));
    pool->tail->process=process;
    pool->tail->arg=arg;
    pool->tail->next=p;
    pool->tail=p;
    pool->queue_size+=1;
    pthread_mutex_unlock(pool->queue_lock);
}

int destroy_thread_pool(struct thread_pool *pool)
{
    struct thread_worker_queue *p=pool->head;
    for(int index=0;index<pool->num;++index)
    {
        ret=pthread_kill(pool->threadid[index], 0);
        
    }
}
