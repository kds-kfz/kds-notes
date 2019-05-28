#include"technician.h"
#include"salesmanager.h"
//main.cpp

void show_link(){
    Employee *p=Employee::head;
    while(p){
	p->show();
	p=p->Getnext();
    }
}

int main(){
    Employee *p;
    Salesmanager sa1("销售经理",4);
    Salesman s1("销售员1",1,100000);
    Technician t1("技术员1",2,120);
    Manager m1("经理1",3);
    Salesman s2("销售员2",1,350000);
    Manager m2("经理2",3);
    Technician t2("技术员2",2,160);
    
    Salesmanager sa2("销售经理",4);
    /*
    p=&s1;
    p->show();
    p=&t1;
    p->show();
    p=&m1;
    p->show();
    p=&sa1;
    sa1.show(s1.Getvolume());
    */
    show_link();
    return 0;
}
