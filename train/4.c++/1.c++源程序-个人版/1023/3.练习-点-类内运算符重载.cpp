#include<iostream>
//3.cpp
//练习：写一个点类

using namespace std;
class Point{
    int x,y;
    public:
    Point() : x(0),y(0){}
    Point(int a,int b) : x(a),y(b){}
    Point(const Point &d) : x(d.x),y(d.y){}
    ~Point(){}

    Point &operator=(const Point &d){
	x=d.x;
	y=d.y;
	return *this;
    }
    //不用&operator，因为本身值不变
    Point operator+(const Point &d){
	/*
	x+=d.x;
	y+=d.y;
	return *this;
	*/
	return Point(x+d.x,y+d.y);
    }
    Point operator-(const Point &d){
	/*
	x-=d.x;
	y-=d.y;
	return *this;
	*/
	return Point(x-d.x,y-d.y);
    }
    Point &operator+=(int a){
//	x+=a;
	y+=a;
	return *this;
    }
    Point &operator-=(int a){
//	x-=a;
	y-=a;
	return *this;
    }
    Point &operator+=(const Point &d){
	x+=d.x;
	y+=d.y;
	return *this;
    }
    Point &operator-=(const Point &d){
	x-=d.x;
	y-=d.y;
	return *this;
    }
    bool operator==(const Point &d)const{
	return x==d.x&&y==d.y;
    }
    bool operator!=(const Point &d)const{
	return x!=d.x||y!=d.y;
//	return !(*this==d);
//	return !d.operator==(*this);
    }
    bool operator>(const Point &d){
	int l1=x*x+y*y;
	int l2=d.x*d.x+d.y*d.y;
	return l1>l2;
    }
    //前++
    Point &operator++(){
	x++;
	y++;
	return *this;
    }
    //后++
    Point operator++(int){
	Point t=*this;
	x++;
	y++;
	return t;
    }
    void show(){
	cout<<"( "<<x<<", "<<y<<" )"<<endl;
    }
};

int main(){
#if 0
    Point d1(2,1);
    Point d2(1,4);
    Point d3;
    d3.show();

    d3=d1-d2;
    d3.show();
    d3=d1+d2;
    d3.show();

    d1+=d2;
    d1.show();
    d2=d1;
    d2.show();
    if(d1==d2)
	cout<<"d1=d2"<<endl;
    d1+=3;
    if(d1>d2)
	cout<<"d1>d2"<<endl;
    d2+=5;
	cout<<"d1<d2"<<endl;
#endif
#if 1
    Point d1(1,3);
    Point d2(3,6);
    cout<<"d1:"<<endl;
    d1.show();
    cout<<"d2:"<<endl;
    d2.show();
    d2=d1++;
    cout<<"d1++:"<<endl;
    d2.show();
    d1.show();

//    d1=d2;

    d2=++d1;
    cout<<"++d1:"<<endl;
    d2.show();
    d1.show();

#endif
    return 0;
}
