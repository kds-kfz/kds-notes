#include<iostream>
//2.cpp
using namespace std;

#if 0
    练习：
    写一个日期类Date
#endif
//struct Date;//即使定义前有类声明也不可以
//Date d1;//错误，在定义对象之前，类必须有完全的定义
//类内的成员函数不区分先后顺序，不需先声明才能使用
struct Date{
    private:
    int year,month,day;
    public:
    //默认函数，无参函数，构造函数，析构，=运算
    
    //算数，关系，位，逻辑运算符
    bool operator==(const Date &d)const{
	return year==d.year&&month==d.month&&day==d.day;
    }
    bool operator!=(const Date &d)const{
	return !(*this==d);
    }
    bool operator>(const Date &d)const{
	return year>d.year||month>d.month||day>d.day;
    }
    bool operator<(const Date &d)const{
//	return year<d.year||month<d.month||day<d.day;
	return !(*this>d||*this==d);
//	return d.operator> (*this); 
//==	d > *this;
    }
    bool operator>=(const Date &d)const{
	return !(*this<d);
    }
    bool operator<=(const Date &d)const{
	return !(*this>d);
    }

    Date &operator-=(const Date &d){
	year-=d.year;
	month-=d.month;
	day-=d.day;
	return *this;
    }
    
    Date &operator-=(int n){
	day-=n;
	return *this;
    }
    /*
    Date &operator=(const Date &d){
	cout<<"operator ="<<endl;
	Date t;
	t.year=d.year;
	t.month=d.month;
	t.day=d.day;
	return *this;
    }
    */
    Date operator-(const Date &d){
	Date t;
	t.year=year-d.year;
	t.month=month-d.month;
	t.day=day-d.day;
	return t;
    }
    Date operator-(int n){
	Date t;
	t.year=year;
	t.month=month;
	t.day=day-n;
	return t;
    }
    
    Date operator+(int n){
	Date t;
	t.year=year;
	t.month=month;
	t.day=day+n;
	return t;
    }

    Date operator+(const Date &d){
	Date t;
	t.year=year+d.year;
	t.month=month+d.month;
	t.day=day+d.day;
	return t;
    }

    Date add(const Date &d){
	Date t;
	t.year=year+d.year;
	t.month=month+d.month;
	t.day=day+d.day;
	if(t.month==4||month==6||month==9||month==11)
	    if(t.day>30){
		t.day=1;
		t.month+=1;
	    }
	if(year_ok()&&t.day>29){
	    t.day=1;
	    t.month+=1;
	}
	else if(!year_ok()&&t.day>28){
	    t.day=1;
	    t.month+=1;
	}
	return t;
    }

    bool year_ok(){
	if(year%4==0&&year%100!=0||!year%400){
//	    cout<<"闰年"<<endl;
	    return true;//闰年29
	}
	else{
//	    cout<<"平年"<<endl;
	    return false;//平年28
	}
    }
    bool month_ok(){
	if(month>0&&month<13)
	    return true;
	else
	    return false;
    }
    bool day_ok(){
	if(day>0&&day<32){
	    if(month==4||month==6||month==9||month==11){
		if(day>30)
		return false;
	    }
	    if(year_ok()&&month==2){
		if(day>29)
		return false;
	    }
	    if(!year_ok()&&month==2){
		if(day>28)
		return false;
	    }
	    return true;
	}
	else
	    return false;
    }
//	int m[31,0,31,30,31,30,31,31,30,31,30,31];
    void show(){
//	if(month_ok()&&day_ok())
	   cout<<year<<"/"<<month<<"/"<<day<<endl;
//	else
//	    cout<<"日期错误"<<endl;
    }
    void date_init(int year,int month,int day){
	this->year=year;
	this->month=month;
	this->day=day;
    }

    void updateday1(){
    if(day_ok()){
	this->day+=1;
	if(month_ok()){
	    if(month==1||month==3||month==5||month==7||
		month==8||month==10||month==12){
		if(day>31){
		day=1;
		month+=1;
		}
		if(month>12){
		day=1;
		month=1;
		year+=1;
		}
	    }
	    else if(month==4||month==6||month==9||month==11){
		if(day>30){
		day=1;
		month+=1;
		}
	    }
	    if(!year_ok()&&month==2){
		if(day>28){//28
		day=1;
		month+=1;
		}
	    }
	    else if(year_ok()&&month==2){
		if(day>29){//29
		day=1;
		month+=1;
		}
	    }
	}
    }
}
    Date getY(){
	return year;
    }
    Date getM(){
	return month;
    }
    Date getD(){
	return day;
    }
};

int main(){
#if 0
    //平28：2003,2015
    //闰29：1976,1900,2008,2012
    Date d1;
    int y,m,d;
    cout<<"输入日期..."<<endl;
    cin>>y>>m>>d;
    d1.date_init(y,m,d);
    d1.show();
    d1.updateday1();
    d1.show();
#endif
#if 0
    Date d1;
    d1.date_init(2017,10,23);

    Date d2=d1;//默认的=运算符，把每个成员直接进行赋值
    //在初始化的时候，=会调用拷贝构造

    d1=d2;//默认=运算符
#if 0
    cout<<"d1:"<<endl;
    d1.show();
    cout<<endl;

    cout<<"d2:"<<endl;
    d2.show();
    cout<<endl;

    cout<<"加法d3=d2.add(d1)"<<endl;
    Date d3;
    d3=d2.add(d1);
    d3.show();
    cout<<endl;

    cout<<"加法d3=d1+d2"<<endl;
    d3=d1+d2;//==d1.opertor+(d2)
    d3.show();
    cout<<endl;

    cout<<"加法d3=d3+20"<<endl;
    d3=d3+20;//Date +int
    d3.show();
    cout<<endl;

    cout<<"减法d3=d3-d2"<<endl;
    d3=d3-d2;
    d3.show();
    cout<<endl;

    cout<<"减法d3=d3-16"<<endl;
    d3=d3-16;
    d3.show();
    cout<<endl;

    cout<<"减法d3-=5"<<endl;
    d3-=5;
    d3.show();
    cout<<endl;


    cout<<"减法d3-=d2"<<endl;
    d3-=d2;
    d3.show();
#endif
    if(d1==d2)
	cout<<"d1==d2"<<endl;
    d2-=1;
    if(d1!=d2)
	cout<<"d1!=d2"<<endl;
    if(d1>d2)
	cout<<"d1>d2"<<endl;
    if(d1>=d2)
	cout<<"d1>=d2"<<endl;
    d1-=4;
    if(d1<d2)
	cout<<"d1<d2"<<endl;
    if(d1<=d2)
	cout<<"d1<=d2"<<endl;
#endif

    return 0;
}
