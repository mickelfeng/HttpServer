#include "threadpool.h"

struct thread_pool * init_thread_pool(int thread_num)
{
    struct thread_pool *pool=(struct thread_pool *)calloc(1,sizeof(struct thread_pool));
    printf("queue_size %d",pool->queue_size);
    pool->thread_num=thread_num;

    pool->head=(struct thread_worker_queue*)calloc(1,sizeof(struct thread_worker_queue));
    pthread_mutex_init(&(pool->queue_lock),NULL);
    pool->tail=pool->head;
    pool->threadid=(pthread_t *)calloc(thread_num,sizeof(pthread_t));
    for(int index=0;index<thread_num;++index)
    {
        int ok=create_detach_thread(pool,index);
        if(ok==-1)
        {
            index--;
            sleep(1);
            continue;
        }
        pool->num+=1;
    }
    return pool;
}

void push_thread_worker(struct thread_pool *pool,void (*process)(void *arg),void *arg)
{
    struct thread_worker_queue *p=(struct thread_worker_queue *)calloc(1,sizeof(struct thread_worker_queue));
    pthread_mutex_lock(&(pool->queue_lock));
    pool->tail->process=process;
    pool->tail->arg=arg;
    pool->tail->next=p;
    pool->tail=p;
    pool->queue_size+=1;
    pthread_mutex_unlock(&(pool->queue_lock));
}

int destroy_thread_pool(struct thread_pool *pool)
{
    struct thread_worker_queue *p;
    for(int index=0;index<pool->num;++index)
    {
        int ok=pthread_cancel(pool->threadid[index]);
        if(ok==0)
        {
            pthread_join(pool->threadid[index],NULL);
            continue;
        }
        index--;
    }
    free(pool->threadid);
    while(pool->head!=NULL)
    {
        p=pool->head;
        pool->head=p->next;
        free(p);
    }
    pthread_mutex_destroy(&(pool->queue_lock));
    free(pool);
    return 0;
}

int create_detach_thread(struct thread_pool *pool,int index)
{
    int ok=0;
    pthread_attr_t attr;
    ok=pthread_attr_init(&attr);
    if(ok!=0)
    {
        return -1;
    }
    ok=pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED);
    if(ok!=0)
    {
        return -1;
    }
    ok=pthread_create(&(pool->threadid[index]),&attr,(void *)thread_run,(void *)pool);
    pthread_attr_destroy(&attr);
    if(ok!=0)
    {
        return -1;
    }
    return 0;
}

void thread_run(void *arg)
{
    struct thread_pool *pool=(struct thread_pool*)arg;
    struct thread_worker_queue *work=NULL;
    while(1)
    {
        pthread_mutex_lock(&(pool->queue_lock));
        if(pool->queue_size!=0)
        {
            work=pool->head;
            pool->head=pool->head->next;
            pool->queue_size--;
        }
        pthread_mutex_unlock(&(pool->queue_lock));
        if(work!=NULL)
        {
            work->process(work->arg);
            free(work);
            work=NULL;
        }
    }
}
