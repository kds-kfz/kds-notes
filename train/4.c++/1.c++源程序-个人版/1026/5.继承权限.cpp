#include<iostream>
#include<cmath>
//5.cpp
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
	/*
//	cout<<"("<<x<<","<<y<<")"<<endl;
	for(int i=x-r;i<=x+r;i++){
	    cout<<"("<<i<<","<<y<<")"<<endl;
	}
	for(int j=y-r;j<=y+r;j++)
	    if(j!=0)
	    cout<<"("<<x<<","<<j<<")"<<endl;
	for(int i=x-r+1;i<=x+r-1;i++){
//	    if(i!=0)
		for(int j=y-r+1;j<=y+r-1;j++)
		    if(j!=x)
		    cout<<"("<<i<<","<<j<<")"<<endl;
	}
	*/
	for(int i=x-r;i<=x+r;i++)
	    for(int j=y-r;j<=y+r;j++){
	    int x1=i-x;
	    int y1=j-y;
	    if(x1*x1+y1*y1<=r*r)
		cout<<"("<<i<<","<<j<<")"<<endl;
	    }
    }
    void show(){
	Point::show();
	cout<<"r = "<<r<<endl;
    }
};

class Ball : Circle{//默认私有继承
    //int x;
    //int y;
    //int r;
    public:
	void show(){
	    cout<<x<<" "<<y<<endl;
	}
};

#if 0
基类的权限	    继承方式	    派生的权限
-------------------------------------------------
public		    public	    public
protected	    public	    protected
private		    public	    不可访问
-------------------------------------------------
public		    protected	    protected
protected	    protected	    protected
private		    protected	    不可访问
-------------------------------------------------
public		    private	    private
protected	    private	    private
private		    private	    不可访问
-------------------------------------------------
#endif

int main(){
    Point p1;
    Point p2(1,1);
//    p1.show();
//    p2.show();
//    Circle c1;
    Circle c2(1,1,1);
//    c1.show();
    c2.show();
    cout<<c2.area()<<endl;
    c2.allPoint();

    return 0;
}

