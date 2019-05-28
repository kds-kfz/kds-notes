#include <iostream>
using namespace std;
//exception 标准库里异常类的基类，内部有一个what()函数，进行一个错误的描写, bad_alloc继承这个基类, 在处理标准库的异常时，可以只用基类异常类型捕获
class Exception1 : public exception {
public:
	const char* what() { return "Exception1"; }
};
int main()
{
	try {
		throw Exception1();
		//new int[222222222222222];
	} 
/*	
	catch (Exception1 & e) {
		cout << e.what() << endl;
	}
*/
	catch (exception & e) {
		cout << e.what() << endl;
	} 

	return 0;
}
