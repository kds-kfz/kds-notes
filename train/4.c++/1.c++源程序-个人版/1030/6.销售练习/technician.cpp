#include"technician.h"

double Technician::pay(){
    return salary=hour*100;
}

void Technician::show(){
    cout<<id<<" "<<name<<" "<<level<<" "<<hour<<" "<<pay()<<endl;
}

