#include "Thread.h"
#include <assert.h>

struct ThreadData {
    using ThreadFunc = std::function<void(void)>;

    ThreadData(const ThreadFunc& func) : func_(func) { }

    void runInThread()
    {
	//异常处理,抛出异常,注意加异常处理的头文件
	try {
	    func_();
	} catch (exception& e) {
	    ::fprintf(stderr, "reason: %s\n", e.what());
	    ::abort();
	} catch (...) {
	    throw;
	}
    }

    ThreadFunc func_;
};

void* startThread(void* obj)
{
    ThreadData* data = (ThreadData*)obj;
    data->runInThread();//将最外层的函数最终传到这个结构中运行
    delete data;
    return NULL;
}

//构造函数,
Thread::Thread(const ThreadFunc& func, std::string name) :
    pthreadId_(0),
    started_(false),
    joined_(false),
    func_(func),//最外层,线程将要执行的函数
    name_(name)
{

}
Thread::~Thread()
{
    //如果线程还在启动,并且没有被回收,将线程分离
    if (started_ && !joined_) {
	pthread_detach(pthreadId_);//析构函数将线程分离,确保线程退出后,资源回收
    }
}

void Thread::start()
{
    assert(!started_);
    started_ = true;

    ThreadData* data = new ThreadData(func_);
    //线程创建,将ThreadData这个结构,作为参数传到线程的运行函数中
    if (pthread_create(&pthreadId_, NULL, startThread, data) != 0) {
	started_ = false;
	delete data;
	::fprintf(stderr, "Failed in pthread_create\n");
	::abort();
    }
}

int Thread::join()
{
    assert(started_);
    assert(!joined_);
    joined_ = true;
    return pthread_join(pthreadId_, NULL);
}




