#include"salesmanager.h"

double Salesmanager::pay(){
    return salary=3000+0.05*total();
}
void Salesmanager::show(){
    cout<<id<<" "<<name<<" "<<level<<" "<<count<<" "<<pay()<<endl;
}

double Salesmanager::pay(double a){
    return salary=3000+0.05*a;
}
void Salesmanager::show(double a){
    cout<<id<<" "<<name<<" "<<level<<""<<count<<" "<<pay(a)<<endl;
}
double Salesmanager::total(){
    double t=0;
    Employee *p=Employee::head;
    while(p){
	if(p->Getlevel()==1)
	    t+=p->Getvolume();
	p=p->Getnext();
    }
    return count=t;
}
