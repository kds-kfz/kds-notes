#include<iostream>
//5.cpp
using namespace std;
int main(){
    int a=10;
    //c++11,获取数据类型
    //编译：g++()5.cpp()-std=c++11
    //定义一个与已知对象相同类型的对象
    decltype (a) b=23;
    
    short s=23;
    decltype (s) c=55;

    cout<<"b_size="<<sizeof(b)<<"\nc_size="<<sizeof(c)<<endl;

    int *p1=&a;	     //内容，指向都可变
    //顶层const
    int *const p2=&a;//指向不可变
    //底层const
    int const *p3=&a;////内容不可变

    int const *const p4=&a;
//	底层	底层

    decltype (p1) px=&b;
    px=&a;
    cout<<"px="<<*px<<endl;
    *px=78;
    cout<<"px="<<*px<<endl;

    decltype (p2) pu=&b;
    *pu=22;
    cout<<"pu="<<*pu<<endl;

    decltype (p3) po=&b;
    po=&a;
    cout<<"po="<<*po<<endl;

    int &r=a;
//    decltype (r) r2;//报错,r2为引用，但是没有初始化
    return 0;
}

