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
//内联函数，是在调用的位置内联的展开, 在编译的时候处理,
//适用于函数比较简单的情况，不能使用递归, inline只是对编译器的一个建议
//在调用之前要有整个函数的具体实现, 所以会把内联函数写在头文件里
//与宏比较：宏是简单替换，在预处理阶段处理
//优点:提高效率, 缺点:增加了代码的长度

inline bool cmpMath(Student s1, Student s2)
{
	return s1.math > s2.math;
}
void Swap(Student& s1, Student& s2)
{
	auto t = s1;
	s1 = s2;
	s2 = t;
}
void sort(Student* arr, int n)
{
	for (int i=0; i<n-1; i++) {
		for (int j=0; j<n-1-i; j++) {
			if (cmpMath(arr[j], arr[j+1])) {
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
	sort(s, 5);
	show(s, 5);

	return 0;
}
