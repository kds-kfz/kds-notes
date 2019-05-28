#include<iostream>
//4.cpp
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
	if(month_ok()&&day_ok())
	   cout<<year<<"/"<<month<<"/"<<day<<endl;
	else
	    cout<<"日期错误"<<endl;
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
};

int main(){
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
    return 0;
}
