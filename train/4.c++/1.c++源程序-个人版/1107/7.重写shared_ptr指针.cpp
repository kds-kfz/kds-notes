#include<iostream>
using namespace std;
//7.重写shared_ptr指针.cpp
template <typename T>
class Shared_ptr{
    T *ptr;
    int *pcount;
    public:
    Shared_ptr() : ptr(NULL),pcount(NULL){}
    Shared_ptr(T *p) : ptr(p){
	pcount=new int(1);
    }
    Shared_ptr(const Shared_ptr &s) : ptr(s.ptr),pcount(s.pcount){
	(*pcount)++;
    }
    void reset(){
	if(pcount==NULL)
	    return;
	if(*pcount==1){
	    delete ptr;
	    delete pcount;
	}
	else if(*pcount>1){
	    (*pcount)--;
	}
	ptr=NULL;
	pcount=NULL;
    }
    int use_count(){
	if(pcount==NULL)
	    return 0;
	return *pcount;
    }
    bool unique(){
	return pcount!=NULL&&*pcount==1;
    }
    void reset(T *p){
	if(pcount!=NULL){
	    if(*pcount==1){
		delete ptr;
		delete pcount;
	    }
	    else if(*pcount>1)
		(*pcount)--;
	}
	ptr=p;
	pcount=new int(1);
    }
    Shared_ptr &operator=(const Shared_ptr &s){
	if(pcount==s.pcout)
	    return *this;
	reset();//ptr==NULL,pcount==NULL
	ptr=s.ptr;
	pcount=s.pcount;
	(*pcount)++;
	return *this;
    }
    ~Shared_ptr(){
	if(pcount!=NULL){
	    if(*pcount==1){
		delete ptr;
		delete pcount;
	    }
	    else if(*pcount>1)
		(*pcount)--;
	}
    }
    T *get()const{
	return ptr;
    }
    T &operator*(){
	return *ptr;
    }
    T *operator->(){
	return ptr;
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
    Shared_ptr<Data>p1(new Data(22));   
    Shared_ptr<Data>p2(p1);   
    Shared_ptr<Data>p3(p2);   
    cout<<"p1_count : "<<p1.use_count()<<endl;
    cout<<"p2_count : "<<p2.use_count()<<endl;
    cout<<"p3_count : "<<p3.use_count()<<endl;
    p1.reset();
    cout<<"p1_count : "<<p1.use_count()<<endl;
    cout<<"p2_count : "<<p2.use_count()<<endl;
    cout<<"p3_count : "<<p3.use_count()<<endl;
    return 0;
}
