#include <iostream>
using namespace std;
//写一个类 Point

/*
1 运算符重载，操作数必须至少有一个自定义的类型,　为了阻止用户更改基本数据运算规则
2 不能改变操作符本身的特性, 不能将二元运算符改成三元运算符, 优先级不能改变
3 不能创建新的运算符 `  ￥ 
4 有的运算符不能重载, sizeof() , ?: .
5 大多数运算符可以在类内重载，也可以在类外重载 + == 
	有的必须在类外重载 >> <<
	有的必须在类内重载 = []        () -> * ++ --
						下标运算符　调用运算符	
*/

/*
Date d1, d2;  & 二元　位运算
d2 & d1;	可以重载
&d1;		& 取址　不能重载
*/


class Point
{
	int x;
	int y;
public:
	Point(int a, int b) : x(a), y(b) { }
	Point() : Point(0, 0) { }
	Point(const Point& p) : x(p.x), y(p.y) { }
	~Point() { }
	Point& operator=(const Point& p) {
		x = p.x;
		y = p.y;
		return *this;
	}
	/*
	Point operator+(const Point& p) {
		return Point(x+p.x, y+p.y);
	}
	*/
	Point operator-(const Point& p) {
		return Point(x-p.x, y-p.y);
	}
	Point& operator+=(const Point& p) {
		x += p.x;
		y += p.y;
		return *this;
	}
	Point& operator-=(const Point& p) {
		x -= p.x;
		y -= p.y;
		return *this;
	}
	bool operator==(const Point& p) {
		return x==p.x && y==p.y;
	}
	bool operator!=(const Point& p) {
		return x!=p.x || y!=p.y;
	}
	bool operator>(const Point& p) {
		int l1 = x*x + y*y;
		int l2 = p.x*p.x + p.y*p.y;
		return l1 > l2;
	}
	bool operator<(const Point& p) {
		int l1 = x*x + y*y;
		int l2 = p.x*p.x + p.y*p.y;
		return l1 < l2;
	}
	void show() {
		cout << "(" << x << ", " << y << ")" << endl;
	}
	Point& operator++() {
		x++;
		y++;
		return *this;
	}
	Point operator++(int) {
		Point t = *this;
		x++;
		y++;
		return t;
	}
	int getX() {
		return x;
	}
	friend Point operator+(const Point& p1, const Point& p2);
	friend Point operator+(const Point& p1, int n);
	friend istream& operator>>(istream& in, Point& p);
	friend ostream& operator<<(ostream& out, const Point& p);
	int operator[](int n) {
		if (n == 0)
			return x;
		else if (n == 1)
			return y;
		else 
			return 0;
	}
	void operator()() {
		cout << "这是一个可调用的对象" << endl;
	}
	void operator()(const Point& p) {
		x = p.x;
		y = p.y;
	}
};
/*			^y
			| .
			|   .(2, 1)
____________|___________> x
			|
			|
Point p1(2, 1);
Point p2(1, 2);
Point p3 = p1 + p2;// (3, 3)
+ - = += -= == != >(比较点到原点的距离) < 
*/
istream& operator>>(istream& in, Point& p) {
	in >> p.x >> p.y;
	return in;
}
ostream& operator<<(ostream& out, const Point& p) {
	out << "(" << p.x << ", " << p.y << ")";
	return out;
}
Point operator+(const Point& p1, const Point& p2) {
	cout << "类外的+" << endl;
	return Point(p1.x+p2.x, p1.y+p2.y);
}
Point operator+(const Point& p1, int n) {
	cout << "类外的+" << endl;
	return Point(p1.x+n, p1.y+n);
}
//运算符重载必须有一个自定义的类型
int operator+(int a, Point& b) {
	cout << "类外的a+b" << endl;
	return a + b.getX();
}
int main()
{
	Point p1(2, 1);
	Point p2(1, 2);
	cout << p1[0] << endl;
	cout << p2[1] << endl;
	p1();
	p1(p2);
//	p1("ds");	错误, 没有这种重载函数
	cout << "p1 = " << p1 << endl;

/*
	p2 = p1++;
	p2.show();
	p1.show();

	p2 = ++p1;
	p2.show();
	p1.show();

	Point p3;
	p3 = p1 + p2;
	p3.show();
	p3 = p1 - p2;
	p3.show();
	p1 += p2;
	p1.show();
	p2 = p1;
	p2.show();
	if (p1 == p2)
		cout << "p1 == p2" << endl;
	else if (p1 < p2)
		cout << "p1 < p2" << endl;
	else if (p2 > p1)
		cout << "p2 > p1" << endl;
	else 
		cout << "距离相等，不是同一个点" << endl;
*/	
//	cin >> p1;
	cout << "p1 << " << p1 << endl;
	//运算符有两种形式	
	//operator<<(cout, p1);
	//ostream类内我们不能改变, 所以不能使用这种方式重载<<运算符
	//cout.operator<<(p1);	
	cout << p1 + p2 << endl;	
	cout << p1 + 10 << endl; //p1.operator+(10);
	cout << (2 + p1) << endl;	 //operator+(10, p1);
	return 0;
}




