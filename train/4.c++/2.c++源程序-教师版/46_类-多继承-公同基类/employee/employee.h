#ifndef __EMPLOYEE_H_
#define __EMPLOYEE_H_
#include <iostream>
using namespace std;
// #define DEBUG

class Employee
{
protected:
	int id;
	string name;
	int level;
	double salary;
	static int count;
	static Employee* head;
	Employee* next;
public:
	Employee() : Employee("") { }	
	Employee(string n);
	Employee(const Employee& e) = delete;
	Employee& operator=(const Employee& e) = delete;
	virtual void show() = 0;
	//在.h里声明
	//virtual const overrde final ...等关键字写在.h里
	virtual double pay() = 0;
	Employee* getNext() { return next; } 
	int getLevel();
	virtual int getSales() {return 0;}
	static Employee* getHead() { return head; }
	static void showList() {
		Employee* p = head;
		while(p){
			p->show();
			p = p->next;
		}
	}
	virtual ~Employee() { 
#ifdef 	DEBUG	
		cout << "~E " << id << endl; 
#endif
	}

};
#endif /* __EMPLYEE_H_ */
