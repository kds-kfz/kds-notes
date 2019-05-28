/*
练习1
vector<int> v1 = {43, 12, 465, 76, 21, 65};

使用迭代器遍历整个数组,把其中的偶数删除掉

练习2
struct Student {string name; int age; };
vector<Student> v;
增加不定个数的Student, 排序，打印

 */


#include <iostream>
#include <algorithm>
#include <vector>
using namespace std;
struct Student
{
	string name;
	int age;
	bool operator<(const Student& s) const {
		return age < s.age;
	}
};
bool cmpStudent(const Student& s1, const Student& s2)
{
	return s1.name > s2.name;
}

int main()
{
	vector<Student> v;
	v.push_back({"Tom", 12});
	Student s;
	string age;
	while(1) {
		cout << "请输入学生姓名(输入\":q\"结束):" << endl;
		cin >> s.name;
		if (s.name == ":q")
			break;
		cout << "请输入学生的年龄:" << endl;
		cin >> age;
		s.age = atoi(age.c_str());
		v.push_back(s);
	}
	for (auto m : v) 
		cout << m.name << " " << m.age << endl;
	//int arr[4]
	//模板排序, 默认两个参数, 从小到大, 必须重载<运算符
	//前两个参数是要排序的位置,不包含末位，第三个参数是一个函数指针, bool(*p)(Student, Student)
	sort(v.begin(), v.end(), cmpStudent);
	//bool(*)(int, int)
	//sort(a, a+4);	
	for (auto it = v.begin(); it != v.end(); it++)
		cout << it->name << " " << it->age << endl;
	

/*
	vector<int> v1 = {12, 46, 76, 22, 65, 66, 7};
					       	
	for (auto it = v1.begin(); it != v1.end();)
	{
		if (*it%2 == 0)
			it = v1.erase(it);//it指向删除的元素下一个
		else
			it++;
	}
	for (auto m : v1)
		cout << m << " ";
	cout << endl;
*/

	return 0;
}
