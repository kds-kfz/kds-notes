#include "salesman.h"
#include "technician.h"
#include "manager.h"
#include "salesmanager.h"
int main()
{
	SalesMan s1("Jack", 20000);
	SalesMan s2("Jack2", 30000);
	SalesMan s3("Jack3", 40000);
	SalesMan s4("Jack4", 50000);
	SalesMan s5("Jack5", 60000);
	SalesMan s6("Jack6", 70000);
	// s1.show();
	Technician t1("Tom", 20);
	// t1.show();
	Manager m1("Mark");
	// m1.show();
	SalesManager s("Bob");
	//s.show();
	Employee::showList();

	return 0;
}
