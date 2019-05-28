#include<iostream>
#include<memory>
//6.shared_ptr指针.cpp
using namespace std;
#if 0
//智能指针
shared_ptr,weak_ptr,unique_ptr
共享型指针 弱指针   独享型指针
#endif

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
#if 0
    //内部有一个计数器，当多个shared_ptr赋值的时候，
    //内部指向同一个对象，计数器会大于1
    //离开作用域时，shared_ptr的析构函数会检查自己的计数器是不是大于1
    //如果大于1,计数器减1,如果等于1,会释放这个资源
    shared_ptr<Data> p1(new Data(5));
    //use_count()返回内部计数器的值
    cout<<"p1_count : "<<p1.use_count()<<endl;
    cout<<"p1->n : "<<p1->n<<endl;
    cout<<"-----------------------------"<<endl;
    shared_ptr<Data> p2(p1);
    cout<<"p1_count : "<<p1.use_count()<<endl;
    cout<<"p2_count : "<<p2.use_count()<<endl;
    cout<<"p2->n : "<<p2->n<<endl;
    cout<<"-----------------------------"<<endl;
    p1->n=99;
    cout<<"p1->n : "<<p1->n<<endl;
    cout<<"p2->n : "<<p2->n<<endl;
    cout<<"-----------------------------"<<endl;
#endif
#if 1
    shared_ptr<Data> p1(new Data(5));
    shared_ptr<Data> p2(p1);
    shared_ptr<Data> p3(p2);
    shared_ptr<Data> p4(new Data(25));
    //make_shared可以制作shared_ptr返回
    shared_ptr<Data>p8=make_shared<Data>(44);
    //make_pair<int,int>(12,45);
    cout<<"-----------------------------"<<endl;
    cout<<"p1->n : "<<p1->n<<endl;
    cout<<"p2->n : "<<p2->n<<endl;
    cout<<"p3->n : "<<p3->n<<endl;
    cout<<"p4->n : "<<p4->n<<endl;
    cout<<"p1_count : "<<p1.use_count()<<endl;
    cout<<"p2_count : "<<p2.use_count()<<endl;
    cout<<"p3_count : "<<p3.use_count()<<endl;
    cout<<"p4_count : "<<p4.use_count()<<endl;
    cout<<"-----------------------------"<<endl;
    p2=p4;//p2原计数器减1,减到0会释放资源
	  //p2等于p4计数器，同时计数器加1
    cout<<"p2=p4------------------------"<<endl;
    cout<<"p2->n : "<<p2->n<<endl;
    cout<<"p2_count : "<<p2.use_count()<<endl;
    cout<<"p4_count : "<<p4.use_count()<<endl;
    Data *p5=new Data(99);
    cout<<"普通指针 p5->n : "<<p5->n<<endl;
    //智能指针不会识别普通指针申请的对象到底有多少指针指向
    //一旦普通指针被智能指针接管，旧不要再用普通指针类
    shared_ptr<Data>p6(p5);
    //shared_ptr<Data>p7(p5);//错误
    //shared_ptr<Data>p7(p6);//正确
    //cout<<p6.use_count()<<endl;//错误，普通指针没有指针计数器
    //cout<<p7.count()<<endl;//错误，计数器都为1,会double free
    //cout<<p8.use_count()<<endl;
    //unique()判断资源是否是唯一指针指向
    cout<<"p2指针是否是唯一指向 : "<<p2.unique()<<endl;
    cout<<"-----------------------------"<<endl;
#if 0
    //reset()放弃接管资源，旧的资源的计数器减1
    cout<<"p2_count : "<<p2.use_count()<<endl;
    cout<<"p4_count : "<<p4.use_count()<<endl;
    p2.reset();
    cout<<"p2.reset()-------------------"<<endl;
    cout<<"p2_count : "<<p2.use_count()<<endl;
    cout<<"p4_count : "<<p4.use_count()<<endl;
    cout<<"p1_count : "<<p1.use_count()<<endl;
    cout<<"p3_count : "<<p3.use_count()<<endl;
    cout<<"-----------------------------"<<endl;
#endif
#if 1
    Data *px=new Data(77);
    p2.reset(px);//放弃旧的接管资源，然后接手新的资源px
    //px不要再使用资源了
    cout<<"px->n : "<<px->n<<endl;
    Data *py=p2.get();//获得内部Data *类型指针
    cout<<"py->n : "<<py->n<<endl;
//    cout<<"px_count : "<<px.use_count()<<endl;//错误
//    cout<<"py_count : "<<py.use_count()<<endl;//错误
    delete px;
//    delete py;//double free
    cout<<p2.use_count()<<endl;
    cout<<p4.use_count()<<endl;
#endif
#endif
    return 0;
}
