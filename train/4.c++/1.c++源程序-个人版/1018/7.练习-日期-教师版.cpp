#include<iostream>
//date.cpp
using namespace std;

//教师版
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
    bool ok;
    public:
    //添加构造函数
    Date(int y=0,int m=1,int d=1){
	year=y;
	month=m;
	day=d;
	ok=all_ok();
    }
#if 0
    void init(int y,int m,int d){
	year=y;
	month=m;
	day=d;
	ok=all_ok();
    }
#endif
    void show(){
	if(ok)
	    cout<<year<<"/"<<month<<"/"<<day<<endl;
	else
	    cout<<"日期错误"<<endl;
    }
    bool updateday1(){
	if(!ok){
	    cout<<"日期错误，不能+1"<<endl;
	    return false;
	}
	if(day==maxday()&&month==12){
	    day=1;
	    month=1;
	    year+=1;
	}
	else if(day==maxday()){
	    day=1;
	    month+=1;
	}
	else
	    day+=1;
	return true;
    }
    private:
    //类内的成员函数不分先后顺序，不需要先声明再调用
    //求每个月的最大天数
    int maxday(){
	if(month==4||month==6||month==9||month==11)
	    return 30;
	else if(month==2&&is_leap())
	    return 29;
	else if(month==2&&!is_leap())
	    return 28;
	else
	    return 31;
    }
    bool is_leap(){
	return year%4==0&&year%100!=0||year%400==0;
    }
    bool all_ok(){
	if(month<1||month>12)
	    return false;
	if(day<1||day>maxday())
	    return false;
	return true;
    }
};

int main(){
    Date d1;
    d1.show();
    d1.updateday1();
    d1.show();

    Date d2(2017,10,19);
    d2.show();
    d2.updateday1();
    d2.show();
    return 0;
}
