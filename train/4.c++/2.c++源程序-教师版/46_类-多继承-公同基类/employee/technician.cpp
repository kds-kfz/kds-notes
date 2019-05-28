#include "technician.h"

Technician::Technician(string n, int h) : Employee(n), hours(h) {
	level = 2;
#ifdef 	DEBUG
	cout << "Technician " << id << endl;
#endif
}

void Technician::show() {
	cout << "---------兼职技术员---------" << endl;
	cout << "id:" << id << " 姓名:" << name << 
		" 级别:" << level << " 工时:" << hours <<
		" 薪水:" << pay() << endl;
}
double Technician::pay() {
	return salary = 100*hours;
}
