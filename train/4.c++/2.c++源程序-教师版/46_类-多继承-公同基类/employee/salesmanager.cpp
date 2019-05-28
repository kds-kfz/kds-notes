#include "salesmanager.h"

SalesManager::SalesManager(string n) : Employee(n) {
	level = 4;
#ifdef 	DEBUG
	cout << "SalesManager " << id << endl;
#endif
}
void SalesManager::show() 
{
	cout << "---------销售经理---------" << endl;
	cout << "id:" << id << " 姓名:" << name << 
		" 级别:" << level << " 销售额:" << sales <<
		" 薪水:" << pay() << endl;
}
double SalesManager::pay()
{
	Employee* p = Employee::getHead();
	while(p) {
		if (p->getLevel() == 1) 
			sales += p->getSales();
		p = p->getNext();
	}
	return 0.05*sales + 3000;
}
