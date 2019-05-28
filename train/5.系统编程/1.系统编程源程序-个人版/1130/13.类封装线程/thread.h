#ifndef _THREAD_H
#define _THREAD_H
#include<pthread.h>//线程函数头文件
#include<iostream>
#include<stdio.h>
#include<functional>
//thread.h
using namespace std;
using namespace std::placeholders;//函数包装器
using ThreadFunc=function<void(void)>;

struct ThreadData {
    ThreadData(const ThreadFunc& f) : func(f) { }
    void runInThread(){
    //异常处理,抛出异常,注意加异常处理的头文件
	try {
	    func();
	} 
	catch(exception& e){
	    fprintf(stderr, "reason: %s\n", e.what());
	    abort();
	} 
	catch(...){
	    throw;
	}
    }
    ThreadFunc func;
};
//基类 
class noncpyable{
    public:
    noncpyable()=default;//默认系统无参构造
    noncpyable(const noncpyable& n)=delete;//禁止合成拷贝构造
    noncpyable& operator=(const noncpyable& )=delete;//禁止赋值
};
//派生类
class Thread : public noncpyable{
    pthread_t ThreadId;//线程的id
    string name;//线程名字
    bool s;//判断是否启动线程
    bool j;//判断线程是否回收
    public:
    ThreadFunc func;//function<void(void)>类型的函数
    Thread(const ThreadFunc& f,string n) : 
	ThreadId(0),name(n),s(0),j(0),func(f){}// == function<void(void)>fun=f
    void start();
    int join();
    pthread_t getpth(){
	printf("pth : %lu\n",ThreadId);
    }
    ~Thread(){
    //如果线程还在启动,并且没有被回收,将线程分离
	if(s&&!j){
	//析构函数将线程分离,确保线程退出后,资源回收
	pthread_detach(ThreadId);
	}
    }
};
//其它函数声明
void* tartThread(void* obj);
#endif
