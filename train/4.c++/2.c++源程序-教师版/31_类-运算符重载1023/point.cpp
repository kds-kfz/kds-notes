#include <iostream>
using namespace std;
//写一个类 Point
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
	Point operator+(const Point& p) {
		return Point(x+p.x, y+p.y);
	}
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

int main()
{
	Point p1(2, 1);
	Point p2(1, 2);

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

	return 0;
}




