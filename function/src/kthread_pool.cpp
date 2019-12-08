#include<cstdio>
#include<cstdlib>
#include<unistd.h>
#include<errno.h>
#include<cstring>

#include<assert.h>
#include<iostream>
#include"kthread_pool.hpp"
using namespace std;

pool_t *KPTHREADPOOL::g_tphread_pool = NULL;

KPTHREADPOOL::KPTHREADPOOL(){}
KPTHREADPOOL::~KPTHREADPOOL(){}

//线程池初始化
bool KPTHREADPOOL::pool_init(unsigned int thread_num, unsigned int queue_max_num){
    int ret;
    g_tphread_pool = new pool_t;
    assert(g_tphread_pool);

    if(pthread_mutex_init(&(g_tphread_pool->mutex), NULL)){
        cout<<"mutex init error\n";
        return false;
    }
    if(pthread_cond_init(&(g_tphread_pool->cond), NULL)){
        cout<<"cond init error\n";
        return false;
    }
    if(pthread_cond_init(&(g_tphread_pool->queue_empty), NULL)){
        cout<<"queue_empty init error\n";
        return false;
    }
    if(pthread_cond_init(&(g_tphread_pool->queue_not_full), NULL)){
        cout<<"queue_full init error\n";
        return false;
    }

#if PTHREAD_MODE == 1
    g_tphread_pool->head = NULL;
    g_tphread_pool->queue_num = 0;
#elif PTHREAD_MODE == 2
    g_tphread_pool->task_node.clear();
#endif

    g_tphread_pool->thread_num = thread_num;
    g_tphread_pool->queue_max_num = queue_max_num;
    g_tphread_pool->pool_close = false;

    g_tphread_pool->pth = new pthread_t[thread_num];
    assert(g_tphread_pool->pth);

    for(unsigned int i = 0;i < thread_num; i++){
        int ret = pthread_create(&(g_tphread_pool->pth[i]), NULL, thread_handle, NULL);
        if(ret != 0){
            char buff[32] = {0};
            sprintf(buff, "pthread_create[%d] error %s\n", i, strerror(ret));
            cout<<buff;
            return false;
        }
    }
    return true;
}

#if PTHREAD_MODE == 1
bool KPTHREADPOOL::pool_add_task(void *(*pfunc)(void *arg), void *arg){
    pthread_mutex_lock(&(g_tphread_pool->mutex));
    while(g_tphread_pool->queue_num == g_tphread_pool->queue_max_num && !g_tphread_pool->pool_close){
        pthread_cond_wait(&(g_tphread_pool->queue_not_full), &(g_tphread_pool->mutex));
    }
#elif PTHREAD_MODE == 2
bool KPTHREADPOOL::pool_add_task(pFunc pf, void *arg){
    pthread_mutex_lock(&(g_tphread_pool->mutex));
    while(g_tphread_pool->task_node.size() == g_tphread_pool->queue_max_num && !g_tphread_pool->pool_close){
        pthread_cond_wait(&(g_tphread_pool->queue_not_full), &(g_tphread_pool->mutex));
    }
#endif

    if(g_tphread_pool->pool_close){
        pthread_mutex_unlock(&(g_tphread_pool->mutex));
        cout<<"pool close\n";
        return false;
    }

#if PTHREAD_MODE == 1
    job_t *pjob = new job_t;
    assert(pjob);

    pjob->pfunc = pfunc;
    pjob->arg = arg;
    pjob->next = NULL;
    pjob->next = g_tphread_pool->head;
    
    g_tphread_pool->head = pjob;
    g_tphread_pool->queue_num++;
#elif PTHREAD_MODE == 2
    //接收任务参数
    fParam szTmp;
    szTmp.insert(make_pair(pf, arg));
    
    //添加任务队列
    g_tphread_pool->task_node.push_back(szTmp);
#endif

    pthread_mutex_unlock(&(g_tphread_pool->mutex));
    pthread_cond_signal(&(g_tphread_pool->cond));

    return true;
}

bool KPTHREADPOOL::pool_destroy(){
    //断言，条件为真，正常执行
    assert(g_tphread_pool);

    pthread_mutex_lock(&(g_tphread_pool->mutex));

#if PTHREAD_MODE == 1
    while(g_tphread_pool->queue_num != 0){
        pthread_cond_wait(&(g_tphread_pool->queue_empty), &(g_tphread_pool->mutex));
    }
#elif PTHREAD_MODE == 2
    while(g_tphread_pool->task_node.size() != 0){
        pthread_cond_wait(&(g_tphread_pool->queue_empty), &(g_tphread_pool->mutex));
    }
#endif

    if(g_tphread_pool->pool_close){
        pthread_mutex_unlock(&(g_tphread_pool->mutex));
        return false;
    }

    g_tphread_pool->pool_close = true;
    pthread_mutex_unlock(&(g_tphread_pool->mutex));

    pthread_cond_broadcast(&(g_tphread_pool->cond));
    pthread_cond_broadcast(&(g_tphread_pool->queue_empty));
    pthread_cond_broadcast(&(g_tphread_pool->queue_not_full));

    for(unsigned int i = 0; i < g_tphread_pool->thread_num; i++)
        pthread_join(g_tphread_pool->pth[i], NULL);

    delete []g_tphread_pool->pth;

#if PTHREAD_MODE == 1
    job_t *temp = NULL;
    while(g_tphread_pool->head){
        temp = g_tphread_pool->head;
        g_tphread_pool->head = temp->next;
        delete temp;
    }
#endif

    pthread_mutex_destroy(&(g_tphread_pool->mutex));
    pthread_cond_destroy(&(g_tphread_pool->cond));
    pthread_cond_destroy(&(g_tphread_pool->queue_empty));
    pthread_cond_destroy(&(g_tphread_pool->queue_not_full));

    delete g_tphread_pool;
    return true;
}

void *thread_handle(void *arg){
    while(1){
        pthread_mutex_lock(&(KPTHREADPOOL::g_tphread_pool->mutex));

#if PTHREAD_MODE == 1
        while(KPTHREADPOOL::g_tphread_pool->queue_num == 0 && !KPTHREADPOOL::g_tphread_pool->pool_close){
            pthread_cond_wait(&(KPTHREADPOOL::g_tphread_pool->cond), &(KPTHREADPOOL::g_tphread_pool->mutex));
        }
#elif PTHREAD_MODE == 2
        while(KPTHREADPOOL::g_tphread_pool->task_node.size() == 0 && !KPTHREADPOOL::g_tphread_pool->pool_close){
            pthread_cond_wait(&(KPTHREADPOOL::g_tphread_pool->cond), &(KPTHREADPOOL::g_tphread_pool->mutex));
        }
#endif

        if(KPTHREADPOOL::g_tphread_pool->pool_close){
            pthread_mutex_unlock(&(KPTHREADPOOL::g_tphread_pool->mutex));
            pthread_exit(NULL);
        }

#if PTHREAD_MODE == 1
        KPTHREADPOOL::g_tphread_pool->queue_num--;
        job_t *pjob = KPTHREADPOOL::g_tphread_pool->head;
        
        if(KPTHREADPOOL::g_tphread_pool->queue_num == 0){
            KPTHREADPOOL::g_tphread_pool->head = NULL;
        }else{
            KPTHREADPOOL::g_tphread_pool->head = pjob->next;
        }

        if(KPTHREADPOOL::g_tphread_pool->queue_num == KPTHREADPOOL::g_tphread_pool->queue_max_num - 1){
            pthread_cond_signal(&(KPTHREADPOOL::g_tphread_pool->queue_not_full));
        }

        if(KPTHREADPOOL::g_tphread_pool->queue_num == 0){
            pthread_cond_signal(&(KPTHREADPOOL::g_tphread_pool->queue_empty));
            pthread_mutex_unlock(&(KPTHREADPOOL::g_tphread_pool->mutex));
        }

        pthread_mutex_unlock(&(KPTHREADPOOL::g_tphread_pool->mutex));
        (pjob->pfunc)(pjob->arg);
        delete pjob;
        pjob = NULL;
#elif PTHREAD_MODE == 2
        //取任务第一个任务
        fParam job = KPTHREADPOOL::g_tphread_pool->task_node.at(0);
        vector<fParam>::iterator its = KPTHREADPOOL::g_tphread_pool->task_node.begin();
        KPTHREADPOOL::g_tphread_pool->task_node.erase(its);

        if(KPTHREADPOOL::g_tphread_pool->task_node.size() == KPTHREADPOOL::g_tphread_pool->queue_max_num - 1){
            pthread_cond_signal(&(KPTHREADPOOL::g_tphread_pool->queue_not_full));
        }

        if(KPTHREADPOOL::g_tphread_pool->task_node.size() == 0){
            pthread_cond_signal(&(KPTHREADPOOL::g_tphread_pool->queue_empty));
            pthread_mutex_unlock(&(KPTHREADPOOL::g_tphread_pool->mutex));
        }

        pthread_mutex_unlock(&(KPTHREADPOOL::g_tphread_pool->mutex));
        //开始执行任务
        fParam::iterator it = job.begin();
        (it->first)(it->second);
#endif
    }
}

void *task(void *arg){
    printf("tasking[%ld] ...\n",(long)arg);
    sleep(1);
    return NULL;
}

#if 1
//测试
int main(){
    KPTHREADPOOL task_pool;
    if((!task_pool.pool_init(10,20))){
        cout<<"pool init fail\n";
        exit(-1);
    }
    for(int i=0;i<50;i++){
        if(!task_pool.pool_add_task(task,(void *)(long)i)){
            printf("add task[%d] fail\n",i);
            exit(-1);
        }
    }
    //    sleep(1);
    if(!task_pool.pool_destroy())
        cout<<"pool destroy fail\n";
    else
        cout<<"pool destroy ok\n";

    return 0;
}
#endif
