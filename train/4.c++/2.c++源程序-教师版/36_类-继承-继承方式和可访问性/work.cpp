/*
定义一个类
交通工具
*/
#include <iostream>
using namespace std;

class Vehicle
{
protected:
	int wheels;
	int speed;
	int seats;
public:
	Vehicle() : wheels(0), speed(0), seats(0) { }
	Vehicle(int w, int s, int se) : wheels(w), speed(s),seats(se) { }
};
class Bike : protected Vehicle
{
//	int wheels;
//	int speed;
//	int seats;
	bool load;
	string name;
public:
	Bike() : load(false) { }
	Bike(int w, int sp, int se, bool l, string n) :
		Vehicle(w, sp, se), load(l), name(n) {
	}
	void show() { 
		cout << "自行车 " << endl 
			<< "轮子个数 " << wheels 
			<< " 最大速度 " << speed << "km/h" 
			<< endl;
		cout << "座位个数 " << seats 
			<< " 是否可以载人 " << load 
			<< " 牌子 " << name << endl;
	}
};
class Bus : Vehicle
{
	string energy;	//油，气，电，混合
	string path;	//81	
	bool autoSelling;
public:
	Bus() : autoSelling(false) { }
	Bus(int w, int sp, int se, string e, string p, bool a) : Vehicle(w, sp, se), energy(e), path(p), autoSelling(a) {
	}

	void show() {
		cout << "公交车 " << endl 
			<< "轮子个数 " << wheels 
			<< " 最大速度 " << speed << "km/h" 
			<< endl;
		cout << "座位个数 " << seats 
			<< " 能源 " << energy << endl
			<< "路线 " << path
			<< " 是否自动售票 " << autoSelling 
			<< endl;
	}
};

int main()
{
	Bike b(2, 20, 1, false, "ofo");
	b.show();
	Bus bu(12, 40, 20, "油电混合", "M211", true);
	bu.show();
	return 0;
}
