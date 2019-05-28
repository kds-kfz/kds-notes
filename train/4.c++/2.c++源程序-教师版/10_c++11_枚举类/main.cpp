#include <iostream>
using namespace std;
//enum的成员不能重名
//c++11 enum class 成员可以重名，但是使用的时候需要加域名
enum class Color
{
	Red,
	Blue = 10,
	Yellow,
};
enum class Dir2
{
	Up = 4,
	Down,
	Left, 
	Right
};
enum class Dir
{
	Up,
	Down,
	Left,
	Right
};


int main()
{
	cout << (int)Color::Yellow << endl;
	Dir d = Dir::Up;
	if (d == Dir::Up) {
		cout << "Up" << endl;
	}
	return 0;
}
