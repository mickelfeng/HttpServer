#include "threadpool.h"

struct thread_pool * init_thread_pool(int min_thread_num,int max_thread_num)
{
    struct thread_pool *pool=(struct thread_pool *)calloc(1,sizeof(struct thread_pool));
    printf("queue_size %d",pool->queue_size);
    pool->min_thread_num=min_thread_num;
    pool->max_thread_num=max_thread_num;

    pool->head=(struct thread_worker_queue*)calloc(1,sizeof(struct thread_worker_queue));
    pthread_mutex_init(&(pool->queue_lock),NULL);
    pthread_cond_init(&(pool->queue_ready),NULL);

    pool->tail=pool->head;
    pool->threadid=(pthread_t *)calloc(max_thread_num,sizeof(pthread_t));
    for(int index=0;index<min_thread_num;++index)
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

void push_thread_worker(struct thread_pool *pool,void *(*process)(void *arg),void *arg)
{
    struct thread_worker_queue *p=(struct thread_worker_queue *)calloc(1,sizeof(struct thread_worker_queue));
    pthread_mutex_lock(&(pool->queue_lock));
    pool->tail->process=process;
    pool->tail->arg=arg;
    pool->tail->next=p;
    pool->tail=p;
    pool->queue_size+=1;
    pthread_mutex_unlock(&(pool->queue_lock));
    pthread_cond_signal(&(pool->queue_ready));//唤醒一个线程处理
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
    pthread_cond_destroy(&(pool->queue_ready));
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
    ok=pthread_create(&(pool->threadid[index]),&attr,(void *)thread_routine,(void *)pool);
    pthread_attr_destroy(&attr);
    if(ok!=0)
    {
        return -1;
    }
    return 0;
}

void thread_pool_keepalive(void *arg)
{
    struct thread_pool *pool=(struct thread_pool *)arg;
    int ok=0;
    while(1)
    {
        for(int index=0;index<pool->min_thread_num;index++)
        {
            ok=pthread_kill(pool->threadid[index],0);//判断线程是否存活
            if(ok==3)//不存在则新建
            {
                ok=create_detach_thread(pool,index);
            }
            if(ok!=0)
            {
                index--;//判断失败或者创建失败，重来
                sleep(1);
            }
        }
        if(pool->queue_size>500)
        {
            for(int index=pool->min_thread_num-1;index<pool->max_thread_num;++index)
            {
                if(pool->threadid[index]==0)
                {
                    ok=create_detach_thread(pool,index);
                    if(ok==-1)
                    {
                        index--;
                        sleep(1);
                        continue;
                    }
                    pool->num+=1;
                }
                else if(pool->threadid[index]!=0)
                {
                    ok=pthread_kill(pool->threadid[index],0);//判断线程是否存活
                    if(ok==3)//不存在则新建
                    {
                        ok=create_detach_thread(pool,index);
                    }
                    if(ok!=0)
                    {
                        index--;//判断失败或者创建失败，重来
                        sleep(1);
                    }
                }
            }
        }
        sleep(30);
    }
}

void* thread_routine(void *arg)
{
    struct thread_pool *pool=(struct thread_pool*)arg;
    struct thread_worker_queue *work=NULL;
    while(1)
    {
        pthread_mutex_lock(&(pool->queue_lock));
        while(pool->queue_size==0)
        {
            pthread_cond_wait(&(pool->queue_ready),&(pool->queue_lock));//等待唤醒
        }
        work=pool->head;
        pool->head=pool->head->next;
        pool->queue_size--;
        pthread_mutex_unlock(&(pool->queue_lock));
        if(work!=NULL)
        {
            work->process(work->arg);
            free(work);
            work=NULL;
        }
    }
}
