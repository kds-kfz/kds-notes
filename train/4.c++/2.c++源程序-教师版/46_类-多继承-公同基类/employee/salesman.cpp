#include "salesman.h"

SalesMan::SalesMan(string n, int s) : Employee(n), sales(s) 
{
	level = 1;
#ifdef 	DEBUG
	cout << "SalseMan " << id << endl;
#endif
}
void SalesMan::show() 
{
	cout << "---------销售员---------" << endl;
	cout << "id:" << id << " 姓名:" << name << 
		" 级别:" << level << " 销售额:" << sales <<
		" 薪水:" << pay() << endl;
}
double SalesMan::pay() {
	return salary = sales*0.03 + 2000;
}
int SalesMan::getSales() {
	return sales;
}