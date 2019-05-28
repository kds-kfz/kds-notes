#include<iostream>
//3.cpp
//练习：写一个点类
//2017/10/24
//运算符重载

using namespace std;
class Point{
    int x,y;
    public:
    Point() : x(0),y(0){}
    Point(int a,int b) : x(a),y(b){}
    Point(const Point &d) : x(d.x),y(d.y){}
    ~Point(){}
    #if 1
    //2017/10/24
    //1.运算符重载，操作数必须至少有1个自定义的类型，
    //为了阻止用户更改基本数据运算规则
    //2.不能改变操作符本身的特性，不能将二元运算符改成3元运算符，优先级不能改变
    //3.不能创建新的运算符
    //4.有的运算符不能重载，sizeof() ?: , . ; : ::
    //5.大多数运算符可以在类内重载，也可以在类外重载+，==
    //有的必须在类外重载>>,<<
    //有的必须在类内重载=,[]下标运算符,()调用运算符,->,*,++,--
    
    //&：二元运算符，与运算，位运算
    //&：一元运算符，取地址，引用
    //Date d1,d2;
    //d1&d2：可以重载
    //&d1：不可以重载
    friend ostream &operator<<(ostream &out,const Point &p);
    friend istream &operator>>(istream &in,Point &p);
    friend Point operator+(const Point &p1,const Point &p2);
    friend Point operator+(const Point &p,int n);
    friend int operator+(int a,Point &b);

    int operator[](int n){
	if(n==0)
	    return x;
	else if(n==1)
	    return y;
	else
	    return 0;
    }
    void operator()(){
	cout<<"这是1个可调用的对象"<<endl;
    }
    void operator()(int n){
	if(n==1)
	    cout<<"选择正确"<<endl;
	else
	    cout<<"选择错误"<<endl;
    }
    #endif
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
	cout<<"类内+"<<endl;
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
    int getX(){
	return x;
    }
    void show(){
	cout<<"( "<<x<<", "<<y<<" )"<<endl;
    }
};
#if 1
//2017/110/24
ostream &operator<<(ostream &out,const Point &p){
    out<<"("<<p.x<<","<<p.y<<")";
    return out;
}

istream &operator>>(istream &in,Point &p){
    in>>p.x>>p.y;
    return in;
}

Point operator+(const Point &p1,const Point &p2){
    cout<<"类外+"<<endl;
    return Point(p1.x+p2.x,p1.y+p2.y);
}

Point operator+(const Point &p,int n){
    return Point(p.x+n,p.y+n);
}

//运算符重载必须有1个自定义的类型
int  operator+(int a,Point &b){
    return a+b.getX();
}
#endif
int main(){
#if 1
    //2017/10/24
    //运算符有两种形式
    //operator<<(out,p1);
    //ostream类内我们不能改变，所以不能使用这种重载运算符
    //cout.operator<<(p1);
    Point p1(1,2);
    Point p2(2,1);
    cout<<p1<<endl;
    cout<<p2<<endl;
    cout<<"p1+p2="<<p1+p2<<endl;
    cout<<"p1+10="<<p1+10<<endl;
    cout<<"2+p2="<<2+p2<<endl;
    cout<<"--------------------"<<endl;
    cout<<"p1.operator+(p2)  "<<p1.operator+(p2)<<endl;
    cout<<"operator+(p1,p2)  "<<operator+(p1,p2)<<endl;
    cout<<"p1+p2   "<<p1+p2<<endl;

    cout<<p1[0]<<endl;
    cout<<p2[1]<<endl;
    p1();
    p2(2);
#endif
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
#if 0
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
