#include<iostream>
//3.bad_alloc用法.cpp
using namespace std;

int main(){
    int *p=NULL;
    try{
	//阻止new抛出异常,申请空间失败，会返回空指针
//	p=new(nothrow)int[10000000000];
	p=new int[10000000000];
	if(p==NULL)
	    cout<<"申请空间失败"<<endl;
    }
    //bad_alloc一个标准库异常的类
    //exception &bad
    catch(bad_alloc &bad){
	cout<<"    what():"<<bad.what()<<endl;
	cout<<"catch end"<<endl;
	return 0;
    }
    delete []p;
    cout<<"main end"<<endl;
    return 0;
}
