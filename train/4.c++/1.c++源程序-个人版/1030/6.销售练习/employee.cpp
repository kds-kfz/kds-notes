#include"employee.h"

//初始化静态变量写在.cpp里
int Employee::count=1000;
Employee *Employee::head=NULL;

//函数的实现写在.cpp里
Employee::Employee(string n,int l,double v) : 
    id(count++),name(n),level(l),salary(0),volume(v),next(head){
    head=this;
    }

