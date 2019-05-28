#include <iostream>
using namespace std;
class Student
{
	int id;
	string name;
	//定义成私有的构造函数，也可以阻止构造对象
//	Student(const Student& s) {}
public:
	Student(int i, string n) : id(i), name(n) {
	}
//	Student() {}		 //默认的构造函数
	Student() = default; //default c++11 使编译器合成默认的构造函数
	
	//**默认的拷贝构造, 不写也有默认的**//
	//Student(const Student& s) : id(s.id), name(s.name) {}
	Student(const Student& s) = delete; //delete 编译器不合成拷贝构造
	
	//**默认的析构**//
	//~Student() {}	析构函数*不要*写成delete函数, 如果析构函数是被删除的，对象就不会释放
	~Student() = default;
	
	//**默认的赋值运算符**//
	//Student& operator=(const Student& s) {
	//  id = s.id; name = s.name; return *this;
	//}
	Student& operator=(const Student& s) = delete;

	friend ostream& operator<< (ostream& out, const Student& s); 
};

ostream& operator<< (ostream& out, const Student& s) 
{
	out << s.id << " " << s.name;
	return out;
}
int main()
{
	Student s1;
	Student s2(s1);
	s1 = s2;
	cout << s1 << endl;
	cout << s2 << endl;
	return 0;
}
