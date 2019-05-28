#include <iostream>
#include <cstdlib>
using namespace std;
/*
Integer in;
	num
	i   ==== > new int;

*/
//int * p = new int;
class Integer
{
	int num;
	int* i;
public:
	Integer(int n) {
		cout << "i = " << n << endl;
		i = new int(n);
	}
	Integer() {
		cout << "i = 0 " << endl;
		i = new int(0);
	}
	Integer(const Integer& in) {
		cout << "copy " << endl;
		i = new int(*in.i);
	}
	//如果不写析构函数，会有默认的析构函数产生
	//~Integer() { } 
	//析构函数，销毁对象, 离开作用域之后自动调用
	//析构顺序和构造顺序相反
	~Integer() {
		//只处理手动申请的对象
		cout << "delete " << *i << endl;
		delete this->i;
	} //调用完析构函数之后，才会释放本对象的内容, i本身
	void show() {
		cout << *i << endl;
	}
};
int main()
{
//	Integer i1;
//	{
//		Integer i2(9);
		//离开作用域, 调用析构
//	}
//	Integer i3(12);
				//调用构造函数
	Integer* p = new Integer(20); //申请空间，创造对象
//	delete p->i;
	//调用析构函数
	delete p;  //删除对象, 释放空间
//	free(p);	没有调用析构函数 i 指向的空间没有释放

	//c语言的malloc不会调用构造函数
	//free不会调用析构函数
	Integer* q = (Integer*)malloc(sizeof(Integer));
//	*q = 200; // Integer(200); *q 创建了一个对象 调用了构造函数和析构函数
	free(q);
	return 0;
}
/*
struct Student
{
	int id;
	string name;	~string
//	char name[20];
	int* score; // score = new int
	Student * next;
	~Student() {
		delete score;
	}
};

Student* head = new Student;
delete head;
*/
/*
Student* p = (Student*)malloc(sizeof(Student));
p.id = 1;
p.score = malloc(4);
free(p.score)
free(p);
*/
