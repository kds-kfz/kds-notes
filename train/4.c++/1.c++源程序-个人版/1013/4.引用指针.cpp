#include<iostream>
//4.cpp
using namespace std;

void fun(int *a,int n){

}

void fun2(int (&a)[4]){
    cout<<"a_size="<<sizeof(a)<<endl;
}

int main(){
#if 1
    int a=10;
    int b=56;
    int &r=a;
    int *p=&a;

    int *&r1=p;//引用指向整型的指针
//    int &*r1=p;//错误，指向整型的指针引用
    
    const int c=20;

    const int &rc=a;
//  rc=23;
//  a=12;
//    r1=45;//报错，可以引用但不能改变
//    int *const p1=&c;//报错，内容可变，但c是不可变的
    int const *pc=&c;//指向可变，内容不可变
// ==   const int *p1=&c;//指向可变，内容不可变
    
    int *px=(int *)pc;
    *px=199;
    cout<<px<<endl;
    cout<<&c<<endl;
    cout<<*px<<endl;
    //c绑定了常量20,c所在的内存的值不影响c的值
    cout<<"c="<<c<<endl;

    const char *name="Tom";
    name="Mike";
    cout<<(void *)&name[0]<<endl;

    //引用常量，右值引用
    const int &rn =10;//const int rn =10;
    cout<<&rn<<endl;

    int arr[4]={1,2,3,4};

    fun2(arr);
    int (*parr)[4]=&arr;
#endif
//练习：下面的定义是否正确，如果错误，请改正
#if 0
    int value=100;
//    const int &r1=value;//x
    int &r1=value;

//    int &r2=100;//x
    
    int &r3=value;//v

//    int &*r4=value;//x
    int *r4=&value;

//    int &*r5=&value;//x
    int *const &r5=&value;

    const int c=20;
//    int *p1=&c;//x
    const int*p1=&c;

    int const *p2=&c;//v

//    int *const p3=&c;//x
    int const *p3=&c;

    const int *const p4=&c;//v
// ==    const int *p4=&c;//v

    int const *&r6=p1;//v

//    int const *&r7=&value;//x
    int *const &r7=&value;
//引用value地址，指向不能变
    int *const &r8=&value;//v
#endif
    return 0;
}
