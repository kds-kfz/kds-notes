#include <iostream>
using namespace std;

int fun2(int* a, int n) 
{
	
}

int fun(int (&a)[4])//引用一个4个长度的整型数组
{
	cout << "size a = " << sizeof(a) << endl;
	
}
int main()
{
	int a = 4;
	int b = 5;
	int& r = a;
	int* p = &a;

	int*& r2 = p;	//引用一个指针
//	int&* r3 = p;	//语法错误
	
	
	const int c = 2;

	const int& rc = a;
	//rc = 23;
	//a = 12;
	//指向的对象不能变	*p不能变
	int const * pc = &c;
/*	
	int* px = (int*)pc;
	*px = 199;
	cout << px << endl;
	cout << &c << endl;
	cout << *px << endl;
	//c绑定了常量2, c所在的内存里的值不影响c的值
	cout << "c = " << c << endl;
*/
/*
	const char* name = "Tom";
	name = "Mike";
	cout << (void*)&name[0] << endl;	
*/
	//*pc = 12;
	pc = &a;
	//指向不能变		p不能变
	int * const pa = &a;
	//pa = &b;
	int* const& rpa = pa;
	//rpa = &b;

	int const*& rpc = pc;
	//*rpc = 100;
	
	//引用常量，右值引用

	const int& rn = 10;//const int rn = 10;

	cout << &rn << endl;

	int arr[4] = {12, 32, 43, 54};
	fun(arr);
	int (*parr)[4] = &arr;
	return 0;
}

练习:	下面的定义是否正确，如果错误，请改正

int value = 100;			
const int& r1 = value;		

int& r2 = 100;		const int& r2 = 100;
int& r3 = value;	
int&* r4 = value;	/
int&* r5 = &value;	int* r5 = &value;

const int c = 20;	
int* p1 = &c;		int const * p1 = &c;
int const* p2 = &c;
int* const p3 = &c;	/

const int* const p4 = &c;
int const*& r6 = p1;

int const*& r7 = &value;/
//引用了value的地址，指向不能变
int* const& r8 = &value;








