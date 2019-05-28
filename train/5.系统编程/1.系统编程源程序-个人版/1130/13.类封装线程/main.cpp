#include"thread.h"
#include<stdio.h>
#include<functional>
#include <string.h>
//main.cpp
using namespace std;
using namespace std::placeholders;//函数包装器
//普通函数
void fun1(const char *s,int n1,int n2){
    printf("%s %d/%d\n",s,n1,n2);
}

int main(){
    function<void(void)>func=bind(fun1,"today is",2017,12);
    Thread t1(func,"line1");
    Thread t2(func,"line2");
    Thread t3(func,"line3");
    t1.start();
    t1.getpth();
    t2.start();
    t2.getpth();
    t3.start();
    t3.getpth();
    int ret=t1.join();
    if(ret!=0)
	printf("rcv : %s\n",strerror(ret));
    ret=t2.join();
    if(ret!=0)
	printf("rcv : %s\n",strerror(ret));
    ret=t3.join();
    if(ret!=0)
	printf("rcv : %s\n",strerror(ret));
    return 0;
}
