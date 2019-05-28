#include <iostream>
#include <cmath>
using namespace std;
struct Triangle
{
	double a, b, c;
	bool isTri() {
		return a+b>c && a+c>b && b+c>a;
	}
	double area() {
		if (!isTri()) 
			return 0;
		double p = a + b + c ;
		p /= 2;
		double s = p * (p - a);
		s *= p - b;
		s *= p - c;
		return sqrt(s);
	}
	double lenth() {
		if (!isTri())
			return 0;
		return a + b + c;
	}
	void show() {
		cout << "三边边长为 : " << a << ", " 
			<< b << ", " << c << endl;
		if (!isTri())
			cout << "不能构成三角形" << endl;
		cout << "周长 " << lenth() << 
			"  面积 " << this->area() << endl;
	}
	void set(double a, double b, double c) {
		this->a = a;
		this->b = b;
		this->c = c;
	}
};

int main()
{
	Triangle t1 = {1, 2, 3};
	t1.show();
	Triangle t2 = {3, 4, 5};
	t2.set(5, 6, 7);
	t2.show();
	
	return 0;
}
