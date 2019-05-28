#include<iostream>
#include<memory>
using namespace std;
//9.weak_ptr指针.cpp
/*
    weak_ptr 相当于shared_ptr的助手
    也能指向shared_ptr,但是不会让计数器增加
    lock()函数可以返回一个shared_ptr
   */

struct B;
struct A{
    A(){cout<<"A"<<endl;}
//    shared_ptr<B>pb;
    weak_ptr<B>pb;
    ~A(){
	cout<<"~A"<<endl;
    }
};
struct B{
    B(){cout<<"B"<<endl;}
    shared_ptr<A>pa;
//    weak_ptr<A>pa;
    ~B(){
	cout<<"~B"<<endl;
    }
};
class Data{
    public:
    int n;
    Data(): Data(0){
	cout<<"Data "<<n<<endl;
    }
    Data(int i):n(i){
	cout<<"Data "<<n<<endl;
    }
    Data(const Data &i) : n(i.n){
	cout<<"copy Data"<<endl;
    }
    ~Data(){
	cout<<"~Data"<<endl;
    }
};
int main(){
    /*
    shared_ptr<A>a(new A());//2 -1 1
    shared_ptr<B>b(new B());//2 -1 1
    a->pb=b;
    b->pa=a;
    cout<<"a_count : "<<a.use_count()<<endl;
    cout<<"b_count : "<<b.use_count()<<endl;
*/
    /*
    //只要一个是weak_ptr，就析构
    shared_ptr<A>a(new A());//1 -1 0
    shared_ptr<B>b(new B());//2 -1 1
    a->pb=b;
    b->pa=a;
    cout<<"a_count : "<<a.use_count()<<endl;
    cout<<"b_count : "<<b.use_count()<<endl;
*/
    /*
    shared_ptr<A>a(new A());//1 -1 0
    shared_ptr<B>b(new B());//2 -1 1
    a->pb=b;
    shared_ptr<B>t(a->pb.lock());
    b->pa=a;
    cout<<"a_count : "<<a.use_count()<<endl;
    cout<<"b_count : "<<b.use_count()<<endl;
    */
    shared_ptr<Data>p1(new Data(80));
    //weak_ptr只能指向shared_ptr,不能指向具体对象
    weak_ptr<Data>p2(p1);
    cout<<p1->n<<endl;
    //use_count()用来查看指向的shared_ptr的计数器
    cout<<p2.use_count()<<endl;
//    p1.reset();
    //lock()从weak_ptr返回一个shared_ptr

    //expired()资源没被释放是0,被释放是1
    cout<<"p2检测资源是否被释放 : "<<p2.expired()<<endl;
    auto p3=p2.lock();//p2->n 不能直接访问shared_ptr内容
    cout<<p3->n<<endl;

    return 0;
}

