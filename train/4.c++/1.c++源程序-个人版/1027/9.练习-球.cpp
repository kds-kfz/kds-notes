#include<iostream>
//7.cpp
using namespace std;
#if 0
练习：
写一个基类Shape抽象类
#endif

//抽象类
class Shape{
    protected:
    double area;
    double volume;
    public:
    //纯虚函数
    virtual double getArea()=0;
    virtual double getVolume()=0;
    Shape() : Shape(0,0){}
    Shape(double a,double b) : area(a),volume(b){}
    //虚析构
    virtual ~Shape(){
	cout<<"~Shape"<<endl;
    }
};
//正方体
class Cube : public Shape{
    double length;
    public:
    Cube() : Cube(0){}
    Cube(double a) : Shape(0,0),length(a){}
    double getArea(){
	return area=length*length*6;
    }
    double getVolume(){
	return volume=length*length*length;
    }
    //析构
    ~Cube(){
	cout<<"~Cube"<<endl;
    }
};
//球体
class Ball : public Shape{
    double r;
    public:
    Ball() : Ball(0){}
    Ball(double a) : Shape(0,0),r(a){}
    double getArea(){
	return area=4*3.14*r*r;
    }
    double getVolume(){
	return volume=r*r*r*3.14*4/3;
    }
    //析构
    ~Ball(){
	cout<<"~Ball"<<endl;
    }
};
//圆柱体
class Cylinder : public Shape{
    double r;
    double high;
    public:
    Cylinder() : Cylinder(0,0){}
    Cylinder(double a,double b) : Shape(0,0),r(a),high(b){}
    double getArea(){
	return area=2*3.14*r*(high+r);
    }
    double getVolume(){
	return volume=r*r*3.14*high;
    }
    ~Cylinder(){
	cout<<"~Cylinder"<<endl;
    }
};

int main(){
    Shape *p;
    double length=0;
    double r=0;
    double high=0;
    cout<<"1.正方体"<<endl;
    cout<<"2.球  体"<<endl;
    cout<<"3.圆柱体"<<endl;
    cout<<"输入选择:"<<endl;
    char choose=0;
    cin>>choose;
    switch(choose){
	case '1':
	    cout<<"输入正方体边长:";
	    cin>>length;
	    p=new Cube(length);
	    cout<<"面积:";
	    cout<<p->getArea()<<endl;
	    cout<<"体积:";
	    cout<<p->getVolume()<<endl;
	    delete p;
	    break;
	case '2':
	    cout<<"输入球体半径:";
	    cin>>r;
	    p=new Ball(r);
	    cout<<"面积:";
	    cout<<p->getArea()<<endl;
	    cout<<"体积:";
	    cout<<p->getVolume()<<endl;
	    delete p;
	    break;
	case '3':
	    cout<<"输入圆柱体底半径:";
	    cin>>r;
	    cout<<"输入圆柱体高:";
	    cin>>high;
	    p=new Cylinder(r,high);
	    cout<<"面积:";
	    cout<<p->getArea()<<endl;
	    cout<<"体积:";
	    cout<<p->getVolume()<<endl;
	    delete p;
	    break;
	defalut:break;
    }

    return 0;
}
