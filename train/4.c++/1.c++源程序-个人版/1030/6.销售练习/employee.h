#ifndef _EMPLOYEE_H_
#define _EMPLOYEE_H_
#include"iostream"
//#include"technician.h"
using namespace std;
//抽象类
class Employee{
    protected:
    int id;
    string name;
    int level;
    double salary;
    static int count;
    Employee *next;
    double volume;
    public:
    static Employee *head;
    //virtual const override final ... 等等关键字写在.h里
    virtual void show()=0;
    virtual double pay()=0;
    Employee(const Employee &e)=delete;
    Employee &operator=(const Employee &e)=delete;
    Employee() : Employee("--",0,0){}
    Employee(string n,int l,double v);
    Employee *Getnext(){return next;}
    int Getlevel(){return level;}
    double Getvolume(){return volume;}
    virtual ~Employee(){cout<<"~Employee"<<endl;}
};


#endif /* _EMPLOYEE_H_ */
