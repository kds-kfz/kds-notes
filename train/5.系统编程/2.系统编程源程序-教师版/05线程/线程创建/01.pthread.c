/*从进程联想到线程*/
线程和进程的关系:
线程是最小的执行单元,进程是资源管理的最小单位


线程概念:
在linux中轻微轻量级进程,本质上仍然是进程

线程之间共享和非共享:

优缺点

线程控制原语:
pthread_self[获取自身ID]
pthread_create[线程创建]
pthread_exit[线程退出]
pthread_join[线程回收]
pthread_detach[线程分离]
pthread_cancle[线程取消]
线程属性:
	修改线程属性的方法
pthread_mutex_init
pthread_mutex_lock
pthread_mutex_unlock
pthread_cond_init
pthread_cond_wait
pthread_cond_broadcast
pthread_cond_signal	

