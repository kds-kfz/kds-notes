#ifndef _SALES_MANAGER_H_
#define _SALES_MANAGER_H_ 
#include "salesman.h"
#include "manager.h"
class SalesManager : public SalesMan, public Manager
{
public:
	SalesManager() : SalesManager("") {}
	SalesManager(string n) ;
	~SalesManager() {
#ifdef 	DEBUG	 
		cout << "~SalesManager " << id << endl;
#endif
	}
	void show() override;
	double pay() override;
};
#endif