#include<iostream>
#include<cmath>
//4.cpp
using namespace std;
#if 0
练习:
    基类:Point
	 x,y
	 getX(),getY()
	 <<
    派生:Circle
	 r
	 getR()
	 area()
    显示在圆内的所有整数点坐标allPoint()
    析构，构造，显示，<<
#endif
class Point{
    protected:
    int x;
    int y;
    public:
    Point() : Point(0,0){
	cout<<"无参 Point"<<endl;
    }
    Point(int a,int b) : x(a),y(b){
	cout<<"有参 Point"<<endl;
    }
    int getX(){
	return x;	
    }
    int getY(){
	return y;
    }
    void show(){
	cout<<"("<<x<<","<<y<<")"<<" ";
    }

};

class Circle : public Point{
    int r;
    public:
    Circle() : Point(0,0),r(0){
	cout<<"无参 Circle"<<endl;
    }
    Circle(int a,int b,int c) : Point(a,b),r(c){
	cout<<"有参 Circle"<<endl;
    }
    int getR(){
	return this->r;
    }
    double area(){
	return 3.14*r*r;
    }
    void allPoint(){
    #if 0
    //个人版
    //不可行，当半径超过4,会出现(-3,-3),(-3)*(-3)+(-3)*(-3)>4*4
    //很麻烦,方法可行但实现太复杂，易出错
    int num=0;
    int n=(int)sqrt(r*r-(y-r+1)*(y-r+1));
    if(y-r+1<0)n*=-1;
//    if(y+r-1<0)n*=-1;
    for(int i=x-r;i<=x+r;i++){
	num++;
	cout<<"("<<i<<","<<y<<")"<<endl;
    }
    for(int j=y-r;j<=y+r;j++)
	if(j!=y){
	    num++;
	    cout<<"("<<x<<","<<j<<")"<<endl;
	}
    for(int i=x-r+1;i<=x+r-1;i++)
	if(i!=x)
	for(int j=n;j<=-n;j++)
	    if(j!=y){
	    num++;
	    cout<<"("<<i<<","<<j<<")"<<endl;
	    }
    cout<<"num = "<<num<<endl;	
    #endif
    #if 1
    int num=0;
    for(int i=x-r;i<=x+r;i++)
	for(int j=y-r;j<=y+r;j++){
	int x1=i-x;
	int y1=j-y;
	if(x1*x1+y1*y1<=r*r){
	    num++;
	    cout<<"("<<i<<","<<j<<")"<<endl;
	}
	}
	cout<<"num = "<<num<<endl;
    #endif
    }
    void show(){
	Point::show();
	cout<<"r = "<<r<<endl;
    }
};


int main(){
    Point p1;
    Point p2(1,1);
//    p1.show();
//    p2.show();
//    Circle c1;
    Circle c2(0,0,4);
//    c1.show();
    c2.show();
    cout<<c2.area()<<endl;
    c2.allPoint();

    return 0;
}

