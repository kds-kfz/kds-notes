#include <iostream>
using namespace std;

struct Student
{
	int id;
	string name;
};

int main()
{
	Student* p = new Student[4] {
		{1001, "Tom"},
		{1002, "Tom2"},
		{1003, "Tom3"},
		{1004, "Tom4"}
	};
	/*
	int* a = new int[1000000000];//如果申请空间失败，抛出异常，直接中断
	*/

	/*
	p[0] = {1001, "Tom"};
	p[1] = {1002, "Tom2"};
	p[2] = {1003, "Tom3"};
	p[3] = {1004, "Tom4"};
*/
	for (int i=0; i<4; i++) {
		cout << p[i].id << " " << p[i].name << endl;
	}

	delete [] p;

	return 0;
}
