#ifndef _MANAGER_H_
#define _MANAGER_H_
#include"employee.h"

class Manager : virtual public Employee{
    public:
    Manager() : Manager("--",0){}
    Manager(string n,int l,double v=0) : Employee(n,l,v){}
    double pay();
    void show();
    ~Manager(){cout<<"~Manager"<<endl;}
};

#endif
