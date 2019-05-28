#include<iostream>
#include<algorithm>
#include<cstring>
//5.lambda表达式.cpp
using namespace std;

//函数名字(参数列表)返回值类型
auto sum(int a,int b)->int{
    //函数体
    return a+b;
}
/*
lambda 表达式，是一个可调用对象，相当于没有没有名字的函数
[扑获列表](参数列表)->返回类型{函数体}
[可以空](可以空)返回值类型可以省略{可以空}
1.距离短
2.简洁
3.效率高
*/
//		  lambda表达式
int (*p)(int,int)=[](int a,int b)->int{
    return a-b;
};
//分号必须加上

//	    lambda表达式
int (*p1)()=[](){
    return 9;
};
//		   lambda表达式
int (*p2)(int,int)=[](int a,int b){
    return a-b;
};
int main(){
    cout<<p(2,6)<<endl;
    cout<<p1()<<endl;
    cout<<p2(5,2)<<endl;

    int a[4]={89,3,-2,7};
    sort(a,a+4,[](int a,int b){return a>b;});
    for(auto m:a)
	cout<<m<<" ";
    cout<<endl;

    string s[4]={"hello","word","Red","yellow"};
//    sort(s,s+4);//默认比较大小，从小到大
//    sort(s,s+4,[](const string &a,const string &b)//比较长短
//	    {return strlen(a.c_str())>strlen(b.c_str());});
    sort(s,s+4,[](const string &a,const string &b)//比较长短
	    {return a.size()>b.size();});
    for(auto m:s)
	cout<<m<<" ";
    cout<<endl;

    return 0;
}
