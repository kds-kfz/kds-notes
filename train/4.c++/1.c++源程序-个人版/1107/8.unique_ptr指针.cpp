#include<iostream>
#include<memory>
//8.unique_ptr指针.cpp
using namespace std;

#if 0
    unique_ptr：独享型指针
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
	cout<<"~Data "<<n<<endl;
    }
};

int main(){
    unique_ptr<Data>p1(new Data(78));
    unique_ptr<Data>p2;
    //unique_ptr<Data>p3(p1);//不支持拷贝构造
    //p3=p1;//不支持赋值

    Data *p=new Data(0);
    p1.reset();//放弃接管资源，并且delete了资源
    p1.reset(p);//放弃旧资源，接管新资源
    cout<<"p->n : "<<p->n<<endl;

    p=p1.release();//p1放弃接管资源，不delete，返回一个指针
    
    delete p;

    return 0;
}
