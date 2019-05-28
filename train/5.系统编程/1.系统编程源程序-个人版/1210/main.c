#include<stdio.h>
#include"thread_pool.h"

//main.c

int main(){
    Pool *pool=pool_init(10,20);
    for(int i=0;i<20;i++){
	pool_add_task(pool,task,(void *)(long)i);
	printf("%d\n",i);
    }
//    sleep(2);
    if(pool_destroy(pool)==0)
	printf("pool_destroy ok\n");
    return 0;
}


