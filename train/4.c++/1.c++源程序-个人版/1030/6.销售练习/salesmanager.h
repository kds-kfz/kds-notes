#ifndef _SALESMANAGER_H
#define _SALESMANAGER_H
//#include"technician.h"
#include"salesman.h"
#include"manager.h"

class Salesmanager : public Manager,public Salesman{
    double count;
    public:
    Salesmanager() : Salesmanager("--",0){}
    Salesmanager(string n,int l,double v=0) : Employee(n,l,v){}
    double pay();
    void show();
    double pay(double a);
    void show(double a);
    double total();
    ~Salesmanager(){cout<<"~Salesmanager"<<endl;}
};

#endif

