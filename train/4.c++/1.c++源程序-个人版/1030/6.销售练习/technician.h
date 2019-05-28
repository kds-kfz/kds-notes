#ifndef _TECHNICIAN_H_
#define _TECHNICIAN_H_
#include"employee.h"

class Technician : virtual public Employee{
    protected:
    int hour;
    public:
    Technician() : Technician("--",0,0){}
    Technician(string n,int l,int h,double v=0) : Employee(n,l,v),hour(h){}
    void show();
    double pay();
    ~Technician(){cout<<"~Technician"<<endl;}
};


#endif
