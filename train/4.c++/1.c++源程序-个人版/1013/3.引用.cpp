#include<iostream>
//3.cpp
using namespace std;
//c++引用
//值交换
void Swap1(int *a,int *b){
    int t=*a;
    *a=*b;
    *b=t;
}

//int &a=形参1,int &b=形参2
//引用两数据交换
void Swap2(int &a,int &b){
    int t=a;
    a=b;
    b=t;
}

//非引用传递的是值，引用传递的是对象
//返回引用
int &Swap3(int &a,int &b){
    int t=a;
    a=b;
    b=t;
//    return t;//错误，不能以引用的方式返回一个已销毁的局部变量
    return a;//正确
}

int main(){
#if 1
    //c语言中指针
    int b=20;
    int *p1=&b;
    cout<<"b_addr="<<&b<<"\n"<<"p1_addr="<<p1<<endl;

    //c++中的引用
    int a=10;
    int &p=a;//p是a的引用，相当于a的别名
//    int &p2;//报错，引用必须初始化，绑定唯一一个对象
    cout<<"a="<<a<<",p="<<p<<endl;
    p=90;
    cout<<"a_addr="<<&a<<"\n"<<"p_addr="<<&p<<endl;

    cout<<"a="<<a<<",b="<<b<<endl;
    Swap1(&a,&b);
    cout<<"a="<<a<<",b="<<b<<endl;
    Swap2(a,b);
    cout<<"a="<<a<<",b="<<b<<endl;

    Swap3(a,b)=100;//返回a,b交换后的a,将a重新赋值100
    cout<<"a="<<a<<",b="<<b<<endl;
#endif
    return 0;
}
