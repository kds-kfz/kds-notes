/*条件变量本身不是锁,通常与互斥量配合使用


pthread_cond_destroy(&cond)

pthread_cond_wait(&cond,&mutex)

在这个函数之前需要

pthread_cond_t cond;
pthread_cond_init(&cond,NULL)

pthread_mutex_t mutex
pthread_mutex_init(&mutex,NULL)

1.阻塞等待条件变量满足
2.释放已经掌握的互斥锁
这两个为一个原子操作

3.当被唤醒,pthread_cond_wait函数
返回时,解除阻塞并重新申请锁

pthread_cond_signal()
唤醒至少一个阻塞在条件变量上的线程

pthread_cond_broadcast()
唤醒所有当前阻塞的线程
*/







