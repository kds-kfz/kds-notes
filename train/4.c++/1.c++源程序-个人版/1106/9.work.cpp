#include<iostream>
#include<functional>
#include<map>
using namespace std;
using namespace std::placeholders;

//9.work.cpp
//练习：
//使用函数，函数指针，lambda表达式，类，写+ - * /四则运算
//map<char,function<>> m;
//m['+'](2,3);

int sum(int a,int b){return a+b;}

int (*p)(int,int)=[](int a,int b)->int{return a-b;};

class Value{
    public:
    int operator()(int a,int b){
	if(b==0){
	    cout<<"不能除0"<<endl;
	    return 0;
	}
	return a/b;
    }
};
int main(){
    map<char,function<int(int,int)> >m;
    m['+']=sum;
    m['-']=p;
    m['*']=[](int a,int b){return a*b;};
    m['/']=Value();

    cout<<m['+'](2,1)<<endl;
    cout<<m['-'](2,1)<<endl;
    cout<<m['*'](2,1)<<endl;
    cout<<m['/'](2,1)<<endl;
    return 0;
}
