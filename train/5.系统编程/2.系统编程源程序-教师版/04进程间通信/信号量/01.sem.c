/*
	生产者消费者模型:
	A------->产生一种资源(p)
	B------->消费资源(v)
	
	x代表资源,资源的个数可以加,可以减(相当于计数器)

	semget的第一个参数,是指定的唯一标识
	如果多个进程之间交互,可以通过唯一的key值，多个进程可以找到
	这个资源
	key值的产生有三种情况:
		1.系统分配 IPC_PRIVATE
		2.自己指定(但要确保key没有被其他使用)
		3.ftok函数产生一个key值
	semget的第二个参数是信号量集合中的个数,创建了多少个信号量
	semget的第三个参数相当于open函数创建时的权限(参考open记忆)
	
	注意:信号量是内核资源,进程退出,信号量不会销毁
	可以调用销毁信号量的函数,来销毁信号量或是操作系统内核重新启动
*/

#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>

int main()
{
	int semId;
	semId = semget(ftok("sem",1),1,IPC_CREAT|0640);
	if(semId < 0){
		printf("sem creat error.\n");
	}
	printf("creat a sem.\n");

	return 0;
}
