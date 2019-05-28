#include <iostream>
using namespace std;

class B1 {
protected:
	int b1;
	int d;
public:
	void show() {
		cout << "B1 " << b1 << endl;
	}
};
class B2 {
protected:
	int b2;
	int d;
public:
	void show() {
		cout << "B2 " << b2 << endl;
	}
};
class D : public B1, public B2
{
	int d;
public:
	void show() {
		cout << b1 << " " << b2 << " " << d << endl;
		cout << B1::d << " " << B2::d << endl;
	}
};

int main()
{
	D d;
	d.show();
	cout << sizeof(d) << endl;
	return 0;
}
