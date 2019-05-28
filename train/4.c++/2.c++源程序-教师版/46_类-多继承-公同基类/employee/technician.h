#ifndef _TECHNICIAN_H_
#define _TECHNICIAN_H_ 
#include "employee.h"

class Technician : public Employee
{
	int hours;
public:
	Technician() : Technician("", 0) {}
	Technician(string n, int h);
	~Technician() { 
#ifdef 	DEBUG		
		cout << "~Technician " << id << endl;
#endif		
}
	void show() override;
	double pay() override;
};

#endif	