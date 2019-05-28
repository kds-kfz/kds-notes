#include <iostream>
#include <queue>
using namespace std;
/*					1 2 3
	------------------
<-	2 3				  <-
1	------------------
	|				|
	front			back
*/
int main()
{
	queue<int> q;
	q.push(21);
	q.push(32);
	q.push(43);
	cout << q.size() << endl;
	cout << q.front() << endl;
	cout << q.back() << endl;
	q.pop();
	cout << "pop" << endl;
	cout << q.size() << endl;
	cout << q.front() << endl;
	cout << q.back() << endl;
	return 0;
}
