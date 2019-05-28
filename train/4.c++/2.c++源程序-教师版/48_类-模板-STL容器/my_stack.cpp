#include <iostream>
using namespace std;

template <typename T>
class Stack
{
	T* ptr;
	int len;
	int max;
public:
	Stack() : ptr(nullptr), len(0), max(4) {
	}
	//拷贝构造
	~Stack() {
		delete[] ptr;
	}
	int size() {
		return len;
	}
	bool empty() {
		return len == 0;
	}
	void push(const T& t) {
		if (ptr == nullptr) {
			ptr = new T[max];
		} else if (len >= max) {
			max *= 2;
			T* pnew = new T[max];
			for (int i=0; i<len; i++) {
				pnew[i] = ptr[i];
			}
			delete [] ptr;
			ptr = pnew;
		}
		ptr[len++] = t;
	}
	void pop() {
		if (len > 0)
			len--;
	}
	T& top() {
//		if (len == 0)
//			abort();
		if (len == 0)
			return ptr[0];
		return ptr[len-1];
	}
};
/*
class String
{
	char* ptr;
	
};
*/
int main()
{
	Stack<int> s1;
	s1.push(2);
	s1.push(22);
	s1.push(23);
	s1.push(24);
	cout << "size = " << s1.size() << endl;
	cout << "empty = " << s1.empty() << endl;
	cout << "top = " << s1.top() << endl;
	s1.top() = 2222;
	cout << "top = " << s1.top() << endl;
	s1.pop();
	s1.pop();
	cout << "size = " << s1.size() << endl;
	cout << "top = " << s1.top() << endl;
	s1.pop();
	s1.pop();
	cout << "size = " << s1.size() << endl;
	cout << "top = " << s1.top() << endl;
	s1.push(999);
	cout << "size = " << s1.size() << endl;
	cout << "top = " << s1.top() << endl;
	return 0;
}
