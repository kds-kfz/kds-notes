/*
练习:
	基类	Point 
			x, y
			
		getX(), getY() 
			<< 		

	派生类	Circle
			r
		getR()
		area()
		显示出在圆内的所有整数点 allPoint()
	构造，析构，显示，<< 

	*/
#include <iostream>
using namespace std;

class Point
{
protected:
	int x;
	int y;
public:
	Point(int i, int j) : x(i), y(j) { 
		cout << "Point 2 " << x << endl;
	}

	Point() : x(0), y(0) { 
		cout << "Point 0 " << x << endl;
	}
	~Point() {
		cout << "~Point " << x << endl;
	}
	int getX() const {
		return x;
	}
	int getY() const {
		return y;
	}
};

ostream& operator<<(ostream& out, const Point& p)
{
	out << "(" << p.getX() << ", " << p.getY() << ")";
	return out;
}

class Circle : public Point
{
	int r;
public:
	Circle() : r(0) { 
		cout << "Circle 0 " << r << endl; 
	}
	Circle(int x, int y, int i) : Point(x, y), r(i) {
		cout << "Circle 3 " << r << endl; 
	}
	~Circle() {
		cout << "~Circle " << r << endl; 
	}
	int getR() const {
		return r;
	}
	double area() const {
		return 3.14*r*r;
	}
	void allPoint() {
		//圆心(x, y) 半径 r
		int x1, y1;
		for (int i=x-r; i<=x+r; i++) 
			for (int j=y-r; j<=y+r; j++) {
				//Point(i, j) - Point(x, y) 
				x1 = i - x;
				y1 = j - y;
				if (x1*x1 + y1*y1 <= r*r)
					cout << "(" << i << ", " << j << ")" << endl;
			}
	}
};


int main()
{
//	Point p(2, 1);
//	cout << p << endl;
	Circle c(0, 0, 2);
//	cout << c.getX() << endl;
	cout << c.area() << endl;
	c.allPoint();
	return 0;
}

