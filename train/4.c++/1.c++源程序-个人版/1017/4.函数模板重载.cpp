#include<iostream>
//4.cpp
using namespace std;

//模板函数重载
template <typename T>
T Max(T a,T b){
    return a>b?a:b;
}

template <typename T>
T Max(T *a,int n){
    T m=a[0];
    for(int i=1;i<n;i++)
	m=m>a[i]?m:a[i];
    return m;
}
//交换模板函数
/*
template <typename T>
void Swap(T &a,T &b){
    T c=a;
    a=b;
    b=c;
}*/
template <typename T>
void Swap(const T &a,const T &b){
    auto c=*a;
    *a=*b;
    *b=c;
}/*
template <typename T>
void Swap(T *a,T *b){
    T c=*a;
    *a=*b;
    *b=c;
}*/
//交换一个数组中两下标值模板函数
template <typename T>
void Swap(T *a,int b,int c){
    Swap(a[b],a[c]);
}

int main(){
#if 0
    //模板函数重载
    int a[4]={1,2,3,4};
    cout<<Max(2,3)<<endl;
    cout<<Max(a,4)<<endl;
#endif
#if 1
    //练习：
    //1.写一个交换函数模板Swap
    //2.写一个Swap能个交换一个数组中两个位置的值
    int a=5,b=10;
//    string a("hello");
//    string b("world");
    cout<<a<<","<<b<<endl;
    Swap(&a,&b);
    cout<<a<<","<<b<<endl;
/*
    int c[4]={1,2,3,4};
    string d[4]={"hello","yellow","red","World"};
    for(auto &n:c)
	cout<<n<<" ";
    cout<<endl;
    Swap(c,0,3);
    for(auto &n:c)
	cout<<n<<" ";
    cout<<endl;
    for(auto &n:d)
	cout<<n<<" ";
    cout<<endl;
    Swap(c,0,3);
    for(auto &n:d)
	cout<<n<<" ";
    cout<<endl;
    */
#endif
    return 0;
}
