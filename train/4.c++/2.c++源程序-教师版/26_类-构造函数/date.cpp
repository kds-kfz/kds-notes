#include <iostream>
using namespace std;
struct Date;
//Date d;	在定义对象之前，类必须有完全的定义
struct Date
{
private:
	int year, month, day;
	bool ok;
public:
	//添加构造函数
	Date(int y=0, int m=1, int d=1) {
		year = y;
		month = m;
		day = d;
		ok = isOk();
	}
	/*
	void init(int y, int m, int d) {
		year = y;
		month = m;
		day = d;
		ok = isOk();
	}
	*/
	void show() {
		if (ok)
			cout << year << "/" << month << "/" << day << endl;
		else
			cout << "日期错误" << endl;
	}
	bool updateOneDay() {
		if (!ok) {
			cout << "日期错误，不能+1" << endl;
			return false;
		}
		if (day == maxDay() && month == 12) {
			day = 1;
			month = 1;
			year++;
		} else if (day == maxDay()) {
			day = 1;
			month++;
		} else 
			day++;
		return true;
	}
private:
	//类内的成员函数不区分先后顺序, 不需要先声明才能使用
	//求每个月的最大天数
	int maxDay() {
		if (month == 4 || month == 6
				|| month == 9 || month == 11)
			return 30;
		else if (month == 2 && isLeap())
			return 29;
		else if (month == 2 && !isLeap())
			return 28;
		else 
			return 31;
	}
	bool isLeap() {
		return year%4==0 && year%100!=0 || year%400==0;
	}
	bool isOk() {
		if (month < 1 || month > 12)
			return false;
		if (day < 1 || day > maxDay()) 
			return false;
		return true;
	}
};
/*
show();
void show()
{
}
*/
int main()
{
	Date d1;
//	d1.init(2000, 12, 31);
	d1.show();
	d1.updateOneDay();
	d1.show();
	Date d2(2017, 10, 19);
	d2.show();
	d2.updateOneDay();
	d2.show();
	return 0;
}
