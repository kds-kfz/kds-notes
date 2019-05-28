#include<iostream>
//10.cpp
using namespace std;
//访问虚函数表地址，虚函数地址
class Base{
    public:
    virtual void show1(){cout<<"show1"<<endl;}
    virtual void show2(){cout<<"show2"<<endl;}
    virtual void show3(){cout<<"show3"<<endl;}
};

typedef void (*Fun)(void);

int main(){
#if 1
    /*
    Base b;
    Fun p=NULL;
    cout<<"Base : 虚函数表地址 "<<(int *)(&b)<<endl;
    cout<<"Base : 虚函数表第一个函数地址 "<<(int *)*(int *)(&b)<<endl;
    p=(Fun)*((int *)*(int *)(&b)+2);
    p();
    */
    /*
    Base b;
    Base *q=&b;
    cout<<"Base : 对象b的首地址 "<<q<<endl;
    cout << (void*)*(int*)q << endl;
    int *q1=(int *)*(int *)q;
    cout<<"Base : 虚函数表第一个表格地址 "<<q1<<endl;
    cout<<"Base : 虚函数表第一个函数地址 "<<(void*)*q1<<endl;    
    int*q2 =  q1+2; 
    Fun p=(Fun)*q2;
    p();
    */
    Base b;
    Base *q=&b;
    cout<<"Base : 对象b的首地址 "<<q<<endl;
    //虚函数表格地址是连续的，表格里存的虚函数地址是随机的
    //虚函数表格地址和虚函数地址存在代码区
    //对象存在栈区
//    cout << (void*)*(long int *)q << endl;//虚函数表格地址
    //解引用
    long int *q1=(long int *)*(long int *)q;
    cout<<"Base : 虚函数表第一个表格地址 "<<q1<<endl;
    cout<<"Base : 虚函数表第一个函数地址 "<<(void*)*q1<<endl;    
    long int *q2 = q1+1; 
    cout<<"Base : 虚函数表第二个函数地址 "<<(void*)*q2<<endl;    
    long int *q3 = q2+1; 
    cout<<"Base : 虚函数表第三个函数地址 "<<(void*)*q3<<endl;    
    Fun p=(Fun)*q3;
    p();
#endif
    return 0;
}
