#include "employee.h"
//初始化静态变量在.cpp里
int Employee::count = 1000;
Employee* Employee::head = nullptr;

//函数的实现在.cpp里
Employee::Employee(string n) : 
	id(++count), name(n), salary(0), level(0), next(head) 
{
	head = this;
#ifdef 	DEBUG	
	cout << "E " << id << endl;
#endif
}
int Employee::getLevel() {
	return level;
}