#include "manager.h"

Manager::Manager(string n) : Employee(n) {
	level = 3;
#ifdef 	DEBUG
	cout << "Manager " << id << endl;
#endif
}

void Manager::show()
{
	cout << "---------经理---------" << endl;
	cout << "id:" << id << " 姓名:" << name << 
		" 级别:" << level << 
		" 薪水:" << pay() << endl;
}
double Manager::pay() 
{
	return salary = 8000;
}
