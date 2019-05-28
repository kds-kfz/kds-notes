#include<iostream>
#include<memory>
//10.enable_shared_from_this.cpp
using namespace std;
#if 0
struct A{
    shared_ptr<A>getself(){
	return shared_ptr<A>(this);
    }
    A(){
	cout<<"A"<<endl;
    }
    ~A(){
	cout<<"~A"<<endl;
    }
};

int main(){
#if 0
    //释放2次，理论上会double free，但本编译器没识别出来
    A *p0=new A;
    shared_ptr<A>px(p0);
    shared_ptr<A>py(p0);
    cout<<px.use_count()<<endl;//1
    cout<<py.use_count()<<endl;//1
#endif
#if 0
    //释放2次，理论上会double free，但本编译器没识别出来
    shared_ptr<A>p1(new A);
    shared_ptr<A>p2=p1->getself();
    cout<<p1.use_count()<<endl;//1
    cout<<p2.use_count()<<endl;//1
#endif
    cout<<"main end"<<endl;
    return 0;
}
#endif
#if 1
//有一个成员对象，weak_ptr
struct A : public enable_shared_from_this<A>{
    shared_ptr<A>getself(){
//	return shared_ptr<A>(this);
	//返回weak_ptr的lock()方法
	return shared_from_this();
    }
    A(){
	cout<<"A"<<endl;
    }
    ~A(){
	cout<<"~A"<<endl;
    }
};
int main(){
    //这样不会有double free
    //只释放1次
    shared_ptr<A>p1(new A);
    shared_ptr<A>p2=p1->getself();
    cout<<p1.use_count()<<endl;//2
    cout<<p2.use_count()<<endl;//2
    cout<<"main end"<<endl;
    return 0;
}
#endif

