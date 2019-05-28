#include <iostream>
using namespace std;
struct Student
{
	int id;
	string name;
	int math;
	int english;
	int c;
	int total;
};
bool cmpC(Student s1, Student s2)
{
	return s1.c > s2.c;
}
bool cmpMath(Student s1, Student s2)
{
	return s1.math > s2.math;
}
void Swap(Student& s1, Student& s2)
{
	auto t = s1;
	s1 = s2;
	s2 = t;
}
void sort(Student* arr, int n, bool (*p)(Student, Student))
{
	for (int i=0; i<n-1; i++) {
		for (int j=0; j<n-1-i; j++) {
			if (p(arr[j], arr[j+1])) {
				//if (arr[j].c > arr[j+1].c)
				Swap(arr[j], arr[j+1]);
			}
		}
	}
}
void show(Student* arr, int n)
{
	for (int i=0; i<n; i++) {
		cout << arr[i].id << " " << arr[i].name << " " 
			<< arr[i].math << " "<< arr[i].english << " " 
			<< arr[i].c << endl;
	}
}
int main()
{
	Student s[5] = {
		{1001, "Tom", 54, 45, 86},
		{1002, "Tom2", 59, 65, 66},
		{1003, "Tom3", 94, 25, 76},
		{1004, "Tom4", 24, 45, 96},
		{1005, "Tom5", 64, 95, 16}
	};
	show(s, 5);
	cout << "--------------- " << endl;
	sort(s, 5, cmpMath);
	show(s, 5);

	return 0;
}
