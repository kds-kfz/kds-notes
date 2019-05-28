#include<iostream>
//6.捕获列表.cpp
using namespace std;
int c;
int sum(int a,int b){
    cout<<"c = "<<c<<endl;
    return a+b;
}

int main(){
    int d=9;
    int e=3;
    int f;
    //[]捕获列表，可以本作用域上面的变量捕获进来
    //[d]依靠被的形式捕获，默认是只读的，除非在函数后面加mutable
    //&[d]以引用的形式捕获，可变
    //[=]全部以拷贝的形式捕获
    //[&]全部以引用的形式捕获
    //[&,d]除了d是拷贝的，其它全部是引用捕获
    //[=,&d]除了d是引用的，其它全部是拷贝捕获
    //int (*p)(int,int)不匹配带捕获列表的lambda表达式
    auto p=[=,&d](int a,int b)mutable{
//	int d=78;
	d=90;
	e=66;
	cout<<"&c = "<<&c<<" , c = "<<c<<endl;//0
	cout<<"d = "<<d<<endl;
	cout<<"e = "<<e<<endl;
	cout<<"f = "<<f<<endl;//随机值
	cout<<"&f = "<<&f<<" , f = "<<f<<endl;
	return a+b;
    };
    cout<<"2+6 = "<<p(2,6)<<endl;
    cout<<"------------------------------"<<endl;
    cout<<"&c = "<<&c<<" , c = "<<c<<endl;//0
    cout<<"d = "<<d<<endl;
    cout<<"e = "<<e<<endl;
    cout<<"f = "<<f<<endl;//随机值
    cout<<"&f = "<<&f<<" , f = "<<f<<endl;
    return 0;
}
