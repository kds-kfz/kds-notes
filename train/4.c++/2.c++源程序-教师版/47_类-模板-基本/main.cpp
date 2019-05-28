#include <iostream>
using namespace std;
//类模板一般都是作为容器
template <typename T> 
class Value
{
	T i;
public:
//	Value() : i(0) { } 
	Value(T n) : i(n) { }
	void setNum(T n) {
		i = n;
	}
	T getNum() const {
		return i;
	}
};
class Integer 
{
	int i;
public:
	Integer() : i(0) { }
	Integer(int a) : i(a) { }
	void setNum(int n) {
		i = n;
	}
	int getNum() const {
		return i;
	}
	Integer operator+(const Integer& n) {
		return Integer(i+n.i);
	}
};
class Double
{
	double i;
public:
	Double() : i(0.0) { }
	Double(double d) : i(d) { }
	void setNum(double n) {
		i = n;
	}
	double getNum() const {
		return i;
	}
	Double operator+(const Double& n) {
		return Double(i+n.i);
	}
};

int main()
{
	Integer n1(2); // 
	Integer n2(3);
	n1 = n1 + n2;
	cout << n1.getNum() << endl;
	
	Double d1;
	Double d2(9.1);
	d1 = d1 + d2;
	cout << d1.getNum() << endl;

	Value<int> v1(2.1);
	cout << v1.getNum() << endl;
		
	Value<double> v2(1.1);
	cout << v2.getNum() << endl;
	
	Value<string> v3;
	cout << v3.getNum() << endl;


	return 0;
}
