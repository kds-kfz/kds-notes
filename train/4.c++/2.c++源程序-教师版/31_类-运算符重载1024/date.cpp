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
	explicit Date(int y=0, int m=1, int d=1) {
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
		cout << year << "/" << month << "/" << day << endl;
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
public:
	Date operator-(int n) {
		Date t;//(0, 1, 1);
		t.year = year;
		t.month = month;
		t.day = day - n;
		t.ok = t.isOk();
		return t;
	}
	Date operator-(const Date& d) {
		Date t;
		t.year = year - d.year;
		t.month = month - d.month;
		t.day = day - d.day;
		t.ok = t.isOk();
		return t;
	}
	Date operator+(int n) {
		Date t;//(0, 1, 1);
		t.year = year;
		t.month = month;
		t.day = day + n;
		t.ok = t.isOk();
		return t;
	}

	Date operator+(const Date& d) {
		cout << "类内的+ " << endl;
		Date t;
		t.year = year + d.year;
		t.month = month + d.month;
		t.day = day + d.day;
		t.ok = t.isOk();
		return t;
	}
	
	//默认的函数: 无参构造，拷贝构造，析构，= 
	Date& operator=(const Date& d) {
		cout << "operator = " << endl;
		year = d.year;
		month = d.month;
		day = d.day;
		ok = d.ok;
		return *this;
	}
	Date(const Date& d) {
		cout << "copy " << endl;
		year = d.year;
		month = d.month;
		day = d.day;
		ok = d.ok;
	}
	Date& operator-=(int n) {
		day -= n;
		ok = isOk();
		return *this;
	}
	Date& operator-=(const Date& d) {
		year -= d.year;
		month -= d.month;
		day -= d.day;
		ok = isOk();
		return *this;
	}
	//算数，关系，位，逻辑
	bool operator==(const Date& d) {
		cout << "类内的== " << endl;
		return year == d.year && 
			month == d.month &&
			day == d.day;
	}
	bool operator!=(const Date& d) const {
		return !(*this == d);
	}
	bool operator>(const Date& d) const {
		if (year > d.year)
			return true;
		else if (year == d.year && month > d.month)
			return true;
		else if (year == d.year && month == d.month && day > d.day)
			return true;
		return false;
	}
	bool operator<(const Date& d) const {
	//	return !(*this > d || *this == d);
		return d > *this;
	}
	//前加加	operator++()
	Date& operator++()
	{
		year++;
		month++;
		day++;
		return *this;
	}
	//后加加 固定写法 operator++(int)
	Date operator++(int) 
	{
		Date t = *this;
		year++;
		month++;
		day++;
		return t;
	}
	int getY() const {
		return year;
	}
	int getM() const {
		return month;
	}
	int getD() const {
		return day;
	}
	friend ostream& operator<<(ostream& out, const Date& d);
	friend istream& operator>>(istream& in, Date& d);
	//在类内声明一个函数是类的友元函数，这个函数可以访问类的私有成员
	friend Date operator+(const Date& d1, const Date& d2);
	friend Date& operator+=(Date& d1, const Date& d2);
	friend bool operator==(const Date& d1, const Date& d2);
	/*
	Date add(const Date& d) {
		Date t;
		t.year = year + d.year;
		t.month = month + d.month;
		t.day = day + d.day;
		t.ok = t.isOk();
		return t;
	}*/
};
/*
show();
void show()
{
}
*/
/*
d1 + d2 ; 
d1.operator+(d2)
operator+(d1, d2);
*/
//istream std::cin 
//输入运算符重载
istream& operator>>(istream& in, Date& d)
{
	in >> d.year >> d.month >> d.day;
	return in;
}
ostream& operator<<(ostream& out, const Date& d) {
//	out << d.getY() << "/" << d.getM() << "/" << d.getD();
	cout << d.year << "/" << d.month << "/" << d.day;
	return out;
}
	
Date& operator+=(Date& d1, const Date& d2) {
	cout << "类外的+= " << endl;
	d1.year += d2.year;
	d1.month += d2.month;
	d1.day += d2.day;
	return d1;
}
bool operator==(const Date& d1, const Date& d2) {
	cout << "类外的== " << endl;
	return d1.year == d2.year && 
			d1.month == d2.month &&
			d1.day == d2.day;
}
Date operator+(const Date& d1, const Date& d2) {
	cout << "类外的+ " << endl;
	Date t;
	t.year = d1.year + d2.year;
	t.month = d1.month + d2.month;
	t.day = d1.day + d2.day;
	return t;
}

int main()
{
	Date d1;
//	d1.init(2000, 12, 31);
	Date d2(2017, 10, 19);
//	d1 + d2 可以调用类内的+,也可以调用类外的+,如果类内和类外同时存在+，优先调用类内的+
	cout << d1.operator+(d2) << endl;
	cout << operator+(d1, d2) << endl;
	cout << d1 + d2 << endl;
	cin >> d1;	//operator>>(cin, d1);
	cout << "d1 = " << d1 << endl;
	d1 += d2;
	cout << "d1 += d2 " << d1 << endl;
//	d1是非const，　const 的函数是一个接近的函数, 类外的==也是接近的函数
//	d1 == d2　找接近的类型 d1不是const 所以两种版本都可以调用，会有二义性
	cout << "d1 == d2 " << (d1 == d2) << endl;

/*	d2.show();
	cout << __LINE__ << endl;
	Date d3 = d1;	//在初始化的时候, = 会调用拷贝构造
	cout << __LINE__ << endl;
*/
//	d1 = d2;//默认的=运算符,　把每个成员直接进行赋值
//	d1 = d1 + 1;	
	//	Date + Date	
//	d1 = d2.add(d3);
//	d1 = d2 - d3; //等同d2.operator-(d3);
	//	 d2.add(d3);
//	d2 = d2.operator+(d3);
//	d2.show();
//	int a = 5 + 6;//int + int
//	d2 = d2 - 10; //Date + int
	
//	d2 -= d2;     //d2.operator-=(10)
/*	if (d1 > d2) //d1.operator==(d2)
		cout << "> " << endl;
	else if (d1 < d2)
		cout << "< " << endl;
	else if (d1 == d2)
		cout << "== " << endl;
	
*/
	//std::cout 是一个输出对象 ostream 类型
	//<< 输出运算符
	//d1是一个对象 Date类型
//	d1 += d2; // d1.operator+=(d2)
	//operator<<(operator<<(cout, d1), endl);
	cout << d1 << " " << d2 << endl;
	int a = 1 << 2;	 	 // 1是一个int，<<是左移运算符
	(cout << 1) << 2;	 // cout是一个ostream类型, 
					     // << 是输出运算符
	//重载输出运算符函数是一个全局的函数
/*	ostream out;
	out << "你好" << endl;
*/

//	 d2.operator+(10);		  
//	cout << b << endl;
/*	d1.show();
	d2.show();
	
	d2 = d1++;
	cout << "d1++" << endl;
	d1.show();
	d2.show();
	d2 = ++d1;
	cout << "++d1" << endl;
	d1.show();
	d2.show();
*/
	return 0;
}
