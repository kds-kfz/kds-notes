#include <iostream>
#include <memory>
using namespace std;

//共享型指针	弱指针	独享型指针
//shared_ptr weak_ptr unique_ptr
class Data
{
public:
	int n;
	Data() : n(0) { 
		cout << "Data " << n << endl;
	}
	Data(int i) : n(i) { 
		cout << "Data " << n << endl;
	}
	Data(const Data& d) : n(d.n) { 
		cout << "copy Data " << n << endl;
	}
	~Data() {
		cout << "~Data " << n << endl;
	}
};

int main()
{
	int* p = new int(9);
	int* q = p;
	
	delete p;
//	delete q;
	//内部有一个计数器，当多个shared_ptr赋值的时候，内部指向同一个对象，计数器会大于一
	//离开作用域时，shared_ptr的析构函数会检查自己的计数器是不是大于1，如果大于1，计数器减一
	//如果等于1, 会释放这个资源
	shared_ptr<Data> p0;
	shared_ptr<Data> p1(new Data(5));
	//use_count()返回内部计数器的值
	shared_ptr<Data> p2(p1);
	shared_ptr<Data> p3(p2);
	p2 = p3;
	cout << p3.use_count() << endl;

	shared_ptr<Data> p4(new Data(100));
	//make_shared 可以制作shared_ptr返回
	shared_ptr<Data> p8 = make_shared<Data>(28);
	//make_pair<int, int>(12, 32);
	
	cout << "p2 = p4 " << endl;
	p2 = p4;	//p2 原计数器减一，减到０会释放资源
				//p2 等于p4的计数器 同时计数器加一
	cout << "p2->n " << p2->n << endl;
	Data* p5 = new Data(1000);
	//智能指针不会识别普通指针申请的对象到底有多少指针指向, 一旦普通指针被智能指针接管，就不要再用普通指针了
	shared_ptr<Data> p6(p5);
//	shared_ptr<Data> p7(p5);
	cout << p6.use_count() << endl;
//	cout << p7.use_count() << endl;错误，计数器都为１，会double free
	cout << p8.use_count() << endl;
	//unique() 判断资源是否是唯一指针指向
	cout << p2.unique() << endl;
	//reset(), 放弃接管资源，旧的资源的计数器减一
	cout << p2.use_count() << endl;
	cout << p4.use_count() << endl;
	//p2.reset();
	Data* px = new Data(89);
	p2.reset(px);//放弃旧的接管资源，然后接手新的资源px
	//px不要再使用资源了
//	px = nullptr;	
	Data* py = p2.get();//获得内部Data*类型的指针
	cout << "py -> " << py->n << endl;

	cout << p2.use_count() << endl;
	cout << p4.use_count() << endl;
	
	return 0;
}
