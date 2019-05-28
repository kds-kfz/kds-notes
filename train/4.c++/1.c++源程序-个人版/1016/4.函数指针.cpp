#include<iostream>
#include<cstdio>
//4.函数指针.cpp
using namespace std;

int sum(int a,int b){
    return a+b;
}

int sub(int a,int b){
    return a-b;
}

typedef int (*pFun)(int,int);

using pFunc=int (*)(int,int);

typedef int (*pFun1)(int);

using pFunc1=int (*)(int);

//求1~n的和
int my_sum(int n){
    if(n<=0)
	return 0;
    else
	return n+my_sum(n-1);
}

//求n的阶乘
int my_pro(int n){
    if(n<0)
	return -1;
    else if(n<=1)
	return 1;
    else
	return n*my_pro(n-1);
}

int main(){
#if 0
    //函数名字是函数地址
    cout<<(void *)sum<<endl;
    printf("%p\n",sum);
    cout<<sum(1,4)<<endl;

    //int *p(int,int)//返回值是int的指针函数
    //函数指针
    int (*p1)(int,int)=sum;
    int (*p2)(int,int)=&sum;
    int (*p3)(int,int)=*sum;
    cout<<p1(1,4)<<endl;
    cout<<p2(1,4)<<endl;
    cout<<p3(1,4)<<endl;
    
    pFun fp1=sum;
    cout<<fp1(1,6)<<endl;
    
    pFunc fp2=sum;
    cout<<fp2(1,9)<<endl;
#endif
    //练习：写两个函数，求1~n的和，求1~n的积
    //my_pro(4);4*3*2*1
    //my_sum(4);4+3+2+1

    pFun1 p1=my_sum;
    pFunc1 p2=my_pro;

    cout<<p1(5)<<endl;
    cout<<p2(5)<<endl;

    return 0;
}
