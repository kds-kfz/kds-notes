#include <iostream>
#include <map>
#include <functional>
using namespace std;

int sum(int a, int b)
{
	return a + b;
}
int (*p)(int, int) = [](int a, int b)->int
{
	return a - b;
};
class Div
{
public:
	int operator()(int a, int b) {
		if (b == 0) {
			cout << "除数不能为0 " << endl;
			return 0;
		}
		return a / b;
	}
};
int main()
{
	map<char, function<int(int,int)> > m;
	m['+'] = sum;
	m['-'] = p;
	m['*'] = [](int a, int b) {return a*b;};
	m['/'] = Div();
	cout << m['+'](2, 1) << endl;
	cout << m['-'](2, 1) << endl;
	cout << m['*'](2, 1) << endl;
	cout << m['/'](2, 1) << endl;

	return 0;
}
