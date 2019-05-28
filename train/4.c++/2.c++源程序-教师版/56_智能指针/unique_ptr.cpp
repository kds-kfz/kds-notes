#include <iostream>
#include <memory>
using namespace std;

/*
   unique_ptr独享型指针，不支持拷贝构造和赋值

*/
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
	unique_ptr<Data> p1(new Data(9));
	unique_ptr<Data> p2;//不支持拷贝构造
//	p2 = p1;			//不支持赋值
	Data* p = new Data(0);
	//p1.reset();//放弃接管资源，并且delete了资源
	p1.reset(p); //放弃了旧的资源，接管新的资源
	cout << p->n << endl;
	p = p1.release();//p1放弃接管资源，不delete, 返回一个指针
	delete p;

	return 0;
}
