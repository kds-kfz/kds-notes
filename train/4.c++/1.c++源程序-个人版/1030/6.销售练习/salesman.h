#ifndef _SALESMAN_H_
#define _SALESMAN_H_
#include"employee.h"
class Salesman : virtual public Employee{
//    protected:
//    double volume;
    public:
    Salesman() : Salesman("--",0,0){}
    Salesman(string n,int l,double v=0) : Employee(n,l,v){}
    double pay();
    void show();
//    double Getvolume(){return volume;}
    ~Salesman(){cout<<"~Salesman"<<endl;}
};

#endif
