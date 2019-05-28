#include <iostream>
using namespace std;

#define DEBUG
template <typename T>
class Vector
{
	T* ptr;
	int len;
	int max;
public:
	//嵌套类和外部的类没有任何关系，只有位置有关系
	//Vector<> ::Iterator it;
	class Iterator
	{
		T* pi;
	public:
		Iterator() : pi(nullptr) { }
		Iterator(T* p) : pi(p) { }
		bool operator==(const Iterator& it) {
			return pi == it.pi;
		}
		bool operator!=(const Iterator& it) {
			return pi != it.pi;
		}
		Iterator& operator++() {
			pi++;
			return *this;
		}
		Iterator operator++(int) {
			Iterator t = *this;
			pi++;
			return t;
		}
		T& operator*() {
			return *pi;
		}
		T* operator->() {
			return pi;
		}
	};
	Vector() : len(0), max(4) { 
		ptr = new T[max]{};
	}
	Vector(int n) {
		//抛出异常
#ifdef DEBUG
		if (n < 0) abort();
#else
		if (n < 0) n = 0;
#endif
		max = n > 4 ? n : 4;
		len = n;
		//{}　初始化会把每个基本数据类型的成员初始成0
		//把自定义类型初始化成默认值
		ptr = new T[max]{};
	}
	Vector(int n, const T& value) :len(n), max(n) {
			//申请空间的类型是无符号整形	
		ptr = new T[max];
		for (int i=0; i<len; i++)
			ptr[i] = value;
	}
	Vector(const Vector& v) : len(v.len), max(v.max) {
		ptr = new T[max];
		for (int i=0; i<len; i++)
			ptr[i] = v.ptr[i];
	}
	~Vector() {
		delete [] ptr;
	}
	int size() const {
		return len;
	}
	bool empty() const {
		return len == 0;
	}
	void push_back(const T& t) {
		if (len >= max) {
			max *= 2;
			T* p = new T[max]{};
			for (int i=0; i<len; i++) {
				p[i] = ptr[i];
			}
			delete [] ptr;
			ptr = p;
		}
		ptr[len++] = t;
	}
	void pop_back() {
		if (len > 0)
			len--;
	}
	T& operator[](int n) {
		return ptr[n]; //*(ptr+n)
	}
	Vector& operator=(const Vector& v)// = delete;
	{
		if (this == &v)
			return *this;
		delete [] ptr;
		max = v.max;
		len = v.len;
		ptr = new T[max];
		for (int i=0; i<len; i++)
			ptr[i] = v.ptr[i];
		return *this;
	}
	//resize()
	Iterator begin() {
		return Iterator(ptr);
	}
	Iterator end() {
		return Iterator(ptr+len);
	}

};
struct Student
{
	int id;
	string name;
};
int main()
{
	Vector<Student> v1(3, {1001, "Tom"});
//	v1.show();
	auto v2 = v1;
	Vector<int> v3(4);
//	v2.show();
	v1.push_back({1002, "Mike"});
	v1.push_back({1003, "Bob"});
	v1.pop_back();
	v1.pop_back();
	v1.pop_back();
	for (int i=0; i<v1.size(); i++)
	{
		cout << v1[i].id << " " << v1[i].name << endl;
	}
	// begin() end() ++ *
	for (auto m : v1) 
		cout << m.id << ", " << m.name << endl; 
	Vector<int> ::Iterator it = v3.begin();
	*it = 1;
	it++;
	Student* s1 = new Student;
	s1->id;
	for (auto it = v1.begin(); it != v1.end(); it++) {
	//	cout << (*it).id << "-" << (*it).name << endl;
		cout << it->id << "==" << it->name << endl;
	}
//	it1 == it2
	return 0;
}
/*
Vector<Student> v1;
Vector<Student> ::Iterator it = v1.begin();
for (auto it = v1.begin(); it!=v.end(); it++)
{
	cout << it->id << " " << it->name << endl;
}
it = v1.begin();
it == != ++
*it ---> Student
(*it).id
it->id
*/
