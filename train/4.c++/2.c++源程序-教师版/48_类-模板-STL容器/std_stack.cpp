#include <iostream>
#include <stack>
//std::stack
using namespace std;
/*
栈  1 2 3
----------------- <-	
|1  		 top
----------------- -> 3 2
数组
[1  2  3]
[0][1][2]
*/
int main()
{
	stack<int> s1;
	cout << "top = " << s1.top() << endl;
	cout << "size = " << s1.size() << endl;

	s1.push(2);
	s1.push(20);
	s1.push(20);
	s1.push(20);
	s1.push(20);
	s1.push(20);
	s1.push(20);
	s1.push(20);
	s1.push(20);
	s1.push(20);
	s1.push(20);
	
	cout << s1.top() << endl;
	s1.top() = 30;
	cout << "top = " << s1.top() << endl;
	cout << "size = " << s1.size() << endl;
	cout << s1.empty() << endl;
	
	s1.pop();
	cout << "top = " << s1.top() << endl;
	cout << "size = " << s1.size() << endl;
	s1.pop();
	cout << "size = " << s1.size() << endl;
	if (s1.empty())
		cout << "栈为空" << endl;
	else
		cout << "top = " << s1.top() << endl;


	return 0;
}
