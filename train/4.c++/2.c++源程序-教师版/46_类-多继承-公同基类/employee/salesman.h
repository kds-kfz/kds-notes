#ifndef __SALES_MAN_H_
#define __SALES_MAN_H_
#include "employee.h"
class SalesMan : virtual public Employee
{
protected:
	int sales;
public:
	SalesMan() : SalesMan("", 0) { }
	SalesMan(string n, int s);
	~SalesMan() {
#ifdef 	DEBUG 
		cout << "~SalesMan " << id << endl;
#endif
	}
	void show() override;
	double pay() override;
	int getSales() override;
};
#endif
