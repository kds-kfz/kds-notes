#include <iostream>
#include <memory>
using namespace std;

/*
	weak_ptr，相当于shared_ptr的助手，也能指向shared_ptr, 但是不会让计数器增加
	lock() 函数可以返回一个shared_ptr
*/
struct B;
struct A
{
	weak_ptr<B> pb;
	A() { cout << "A" << endl; }
	~A() { cout << "~A" << endl;}
};
struct B
{
	shared_ptr<A> pa;
	B() { cout << "B" << endl; }
	~B() { cout << "~B" << endl;}
};
struct Data
{
	int n;
	Data() : n(0) {
		cout << "Data " << n << endl;
	}
	Data(int i) : n(i) {
		cout << "Data " << n << endl;
	}
	~Data() {
		cout << "~D" << endl;
	}
};
int main()
{
	/*
	shared_ptr<A> a(new A());
	shared_ptr<B> b(new B()); 
	a->pb = b; 
	shared_ptr<B> t(a->pb.lock());	
	b->pa = a;
	cout << a.use_count() << endl;
	cout << b.use_count() << endl;
	*/
	shared_ptr<Data> p1(new Data(9));
	//weak_ptr只能指向shared_ptr, 不能指向具体对象
	weak_ptr<Data> pw(p1);
	cout << p1->n << endl;
	//use_count用来查看指向的shared_ptr的计数器
	cout << pw.use_count() << endl;
	p1.reset();
	//lock() 从weak_ptr返回一个shared_ptr
	cout << "监测的资源有没有释放 " << pw.expired() << endl;
	auto p2 = pw.lock(); //pw->n 不能直接访问shared_ptr的内容
	cout << p2->n << endl;
	
	return 0;
}
