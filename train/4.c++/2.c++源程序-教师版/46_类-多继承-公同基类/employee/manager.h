#ifndef _MANAGER_H_
#define _MANAGER_H_
#include "employee.h"
class Manager : virtual public Employee
{
public:
	Manager() : Manager("") { }
	Manager(string n);
	~Manager() { 
#ifdef 	DEBUG
		cout << "~Manager " << id << endl;
#endif
	}
	void show() override;
	double pay() override;
				
};
#endif