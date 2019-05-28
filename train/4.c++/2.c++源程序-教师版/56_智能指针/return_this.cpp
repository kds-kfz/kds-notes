#include <iostream>
#include <memory>
using namespace std;
/*
struct A
{
	shared_ptr<A> getSelf() {
		return shared_ptr<A>(this);
	}
	A() {cout << "A" << endl;}
	~A() { cout << "~A" << endl;}
};
int main()
{
//	A* p0 = new A;
//	shared_ptr<A> px(p0);
//	shared_ptr<A> py(p0);

	shared_ptr<A> p1(new A);
	shared_ptr<A> p2 = p1->getSelf();

	cout << p1.use_count() << endl;
	cout << p2.use_count() << endl;
	return 0;
}
*/
//有一个成员对象,weak_ptr
struct A : public enable_shared_from_this<A>
{
	shared_ptr<A> getSelf() {
	//	return shared_ptr<A>(this);
		//返回了weak_ptr的lock()方法
		return shared_from_this();
	}
	A() {cout << "A" << endl;}
	~A() { cout << "~A" << endl;}
};
int main() 
{
	shared_ptr<A> p1(new A);
	shared_ptr<A> p2 = p1->getSelf();

	cout << p1.use_count() << endl;
	cout << p2.use_count() << endl;
	return 0;
};
