#include"thread.h"
#include <assert.h>
//thread.cpp
void* startThread(void* obj){   
    ThreadData* data = (ThreadData*)obj;
    data->runInThread();//将最外层的函数最终传到这个结构中运行
    delete data;
    return NULL;
}
void Thread::start(){
    assert(!s);
    s=true;

    ThreadData* data = new ThreadData(func);
    //线程创建,将ThreadData这个结构,作为参数传到线程的运行函数中
    if (pthread_create(&ThreadId, NULL, startThread, data) != 0) {
	s=false;
	delete data;
	fprintf(stderr, "Failed in pthread_create\n");
	abort();
    }
}
int Thread::join(){
    assert(s);
    assert(!j);
    j= true;
    return pthread_join(ThreadId, NULL);
}

