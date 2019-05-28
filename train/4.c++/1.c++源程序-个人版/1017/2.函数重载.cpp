#include<iostream>
//2.cpp
using namespace std;

using U32=unsigned int;

//函数重载，可以根据参数的类型或个数不同，同名函数可以区分不同
//传参可以3个或两个，如果是2个与下面的传参模糊
//函数的默认参数有肯能与函数重载冲突
int sum(int a,int b,int c=0){
    cout<<"int "<<endl;
    return a+b+c;
}

int sum(int a,int b){
    cout<<"int "<<endl;
    return a+b;
}
//注意传参不要有精度缺失，否则错误
double sum(double a,double b){
    cout<<"double "<<endl;
    return a+b;
}

float sum(float a,float b){
    cout<<"double "<<endl;
    return a+b;
}

//练习：可以求int,double,string最大值，函数名字Max
int Max(int a,int b){
    return a>b?a:b;
}

U32 Max(U32 a,U32 b){
    return a>b?a:b;
}

double Max(double a,double b){
    return a>b?a:b;
}

string Max(string a,string b){
    return a>b?a:b;
}

int main(){

    //错误，传参模糊，有两个函数同名且传参都可以是两个整型
//    cout<<sum(1,2)<<endl;
    //错误，传参模糊，int,daouble
//    cout<<sum(1.2,8)<<endl;
    
    cout<<Max(2,9)<<endl;
    cout<<Max(2u,9u)<<endl;
    cout<<Max(-1.2f,-1.8f)<<endl;
    cout<<Max("qqqsd","qqqabdx")<<endl;

    //max，是标准库的模板，会识别成const char*类型,比较的是首字母地址的大小
//    cout<<max("qqqsd","qqqabdx")<<endl;

    return 0;
}
