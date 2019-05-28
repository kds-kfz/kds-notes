#include"salesman.h"
double Salesman::pay(){
    return salary=volume*0.03+2000;
}
void Salesman::show(){
    cout<<id<<" "<<name<<" "<<level<<" "<<volume<<" "<<pay()<<endl;
}


