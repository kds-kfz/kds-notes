#include <iostream>
using namespace std;

int main()
{
	pair<int, string> p;
	p.first = 1001;
	p.second = "Z";
	pair<int, string> p2(1001, "T");
	cout << p.first << " " << p.second << endl;
	cout << p2.first << " " << p2.second << endl;
	//first 和 second 全相等
	//先比较第一个，如果第一个相等, 再比较第二个
	if (p < p2) 
		cout << "<" << endl;
	else 
		cout << ">=" << endl;

	return 0;
}
