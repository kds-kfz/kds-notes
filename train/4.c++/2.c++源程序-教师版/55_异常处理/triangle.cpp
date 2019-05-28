#include <iostream>
using namespace std;
class Triangle
{
	int a;
	int b;
	int c;
public:
	Triangle(int i, int j, int k) : a(i), b(j), c(k) {
		if (!isOk())
			throw "三边不能构成三角形";
		cout << "T" << endl;
	}
	bool isOk() {
		return a+b>c && a+c>b && b+c>a;
	}
	void show() {
		cout << "三角形三边: ";
		cout << a << " " << b << " " << c << endl;
	}
	~Triangle() {
		cout << "~" << endl;
	}
};
int main()
{
	Triangle* p = nullptr;
	try {
		p = new Triangle(1, 4, 5);
	} catch (const char* s) {
		cout << s << endl;
		return 0;
	} 
	p->show();
	delete p;
	cout << "end main()" << endl;
	return 0;
}
