#include <iostream>
using namespace std;

template <typename T>
class Shared_ptr
{
	T* ptr;
	int* pcount;
public:
	Shared_ptr() : ptr(nullptr), pcount(nullptr) {
	}
	Shared_ptr(T* p) : ptr(p){
		pcount = new int(1);
	}
	Shared_ptr(const Shared_ptr& s) : 
		ptr(s.ptr), pcount(s.pcount)
	{	
		(*pcount)++;
	}
	void reset() {
		if (pcount == nullptr)
			return;
		if (*pcount == 1) {
			delete ptr;
			delete pcount;
		} else if (*pcount > 1) {
			(*pcount)--;
		} 
		ptr = nullptr;
		pcount = nullptr;
	}
	void reset(T* p) {
		if (pcount != nullptr) {
			if (*pcount == 1) {
				delete ptr;
				delete pcount;
			} else if (*pcount > 1) {
				(*pcount)--;
			}
		}
		pcount = new int(1);
		ptr = p;
	}
	int use_count() {
		if (pcount == nullptr)
			return 0;
		return *pcount;
	}
	bool unique() {
		return pcount != nullptr && *pcount == 1;
	}
	/*
	T* p
	Shared_ptr<T> p1(p);
	Shared_ptr<T> p2(p);
	p1 = p2;
	Shared_ptr<T> p3(new T);
	Shared_ptr<T> p4(p3);
	p3 = p4;
*/
	Shared_ptr& operator=(const Shared_ptr& s) {
		//自拷贝
		if (pcount == s.pcount) 
			return *this;
		
		reset();//ptr == nullptr, pcount == nullptr
		ptr = s.ptr;
		pcount = s.pcount;
		(*pcount)++;

		return *this;
	}
	~Shared_ptr() {
		if (pcount != nullptr) {
			if (*pcount == 1) {
				delete ptr;
				delete pcount;
			} else if (*pcount > 1) {
				(*pcount)--;
			}
		}
	}
	T* get() const {
		return ptr;
	}
	T& operator*() {
		return *ptr;	
	}
	T* operator->() {
		return ptr;
	}
};
class Data
{
public:
	int n;
	Data(int i) : n(i) {
		cout << "Data " << n << endl;
	}
	~Data() {
		cout << "~Data " << n << endl;
	}
};

int main()
{
	Shared_ptr<Data> p(new Data(9));
	cout << p->n  << endl;
	cout << "p.count " << p.use_count() << endl;
	Shared_ptr<Data> p2(p);
	cout << "p.count " << p.use_count() << endl;
	Shared_ptr<Data> p3;
	cout << "p3.count " << p3.use_count() << endl;
	
	p3 = p;
	p2.reset();
	cout << "p2.count " << p2.use_count() << endl;
	cout << "p3.count " << p3.use_count() << endl;
	

	return 0;
}
