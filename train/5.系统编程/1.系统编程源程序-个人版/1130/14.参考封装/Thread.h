#ifndef THREAD_H
#define THREAD_H

#include "noncpyable.h"
#include <string>
#include <functional>
#include <pthread.h>
//thread.h
//noncpyable禁止合成拷贝构造和赋值
class Thread : noncpyable {
    //typedef std::function<void(void)> ThreadFunc;
    using ThreadFunc = std::function<void(void)>;
    public:
    //Thread构造函数需要传,上面定义的ThreadFunc类型的函数
    Thread(const ThreadFunc& func, std::string name=std::string());
    ~Thread();

    void start();//线程创建
    int join();//线程回收
    private:
    pthread_t pthreadId_;//线程的id

    bool started_;//判断是否启动线程
    bool joined_;//判断线程是否回收
    ThreadFunc func_;//function<void(void)>类型的函数
    std::string name_;
};

#endif
