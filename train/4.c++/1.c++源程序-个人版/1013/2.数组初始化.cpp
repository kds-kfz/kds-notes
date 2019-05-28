#include<iostream>
//2.cpp
using namespace std;
int main(){
    //c语言数组初始化方式
    int a1[4];//没有()初始化数组
    int a2[4]={1,2,3,4};//列表初始化
    int a3[4]={};
    int a4[4]={1,2,3};
    int a5[]={};//空格数组
    int a6[0];
    cout<<"a4_addr="<<&a4<<endl;
    cout<<"a5_addr="<<&a5<<endl;
    cout<<"a6_addr="<<&a6<<endl;

    cout<<"a5_size="<<sizeof(a5)<<endl;//0
    cout<<"a6_size="<<sizeof(a6)<<endl;//0
    //c++11数组初始化
    int b1[4]={1,2,3,4};//列表初始化
    int b2[4]={};
    for(int i=0;i<4;i++)
	cout<<b1[i]<<" ";
    cout<<endl;
    return 0;
}
