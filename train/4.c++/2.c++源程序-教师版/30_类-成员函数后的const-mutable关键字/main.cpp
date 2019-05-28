#include <iostream>
using namespace std;

class Integer
{
	int* num;
public:
	//mutable 定一个变量是可变的
	mutable int sum;
	//const 定义一个变量是不可变的
//	const int n;
	Integer() {
		num = new int(0);
	}
	Integer(int n) {
		num = new int(n);
	}
	Integer(const Integer& i) {
		num = new int(i.getNum());
	//	num = new int(*i.num);
	}
	~Integer() {
		delete num;
	}
	//在成员函数的后面加const, 意思是这个函数不能改变类的成员变量
	//const成员函数可以重载
	int getNum() const {
	//	num = new int(12);
	//	sum = 12;
		cout << "const " << endl;
		return *num;
	}
	//const 加在前面，意思是返回值是const的
	int getNum() {
		cout << "non - const " << endl;
		return *num;
	}

	void setNum(int n) {
		*num = n;
	}
};
/*	只有成员函数可以加const属性
int fun() const 错误
{
}
*/
int main()
{
	Integer i1(9);
	cout << i1.getNum() << endl;
	i1.setNum(19);
	cout << i1.getNum() << endl;
	
	const Integer i2(8);//i2是不可改变的
//	i2.setNum(4);		//i2的函数有可能改变它的内容
			//所以我们调用的函数不能改变i2的成员变量
	cout << i2.getNum() << endl;
//	i2.sum = 10;
	cout << i2.sum << endl;

	return 0;
}
