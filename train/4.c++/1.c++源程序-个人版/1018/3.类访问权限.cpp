#include<iostream>
#include<cmath>
//3.cpp

//类访问权限
using namespace std;
#if 0
    写一个三角形类
    成员变量：三边
    成员函数：判断三角形能不能成立
    求三角形面积和周长
#endif
struct Delta{
    float l,w,h;
    bool judge(){
	if(l+w>h && l+h>w && w+h>l)
	    return true;
	else
	    return false;
    }
    float girth(){
	if(!judge())
	    return 0;
	return this->l+this->w+this->h;
    }
    float area(){
	if(!judge())
	    return 0;
	float s=(l+w+h)/2;
	return sqrt(s*(s-l)*(s-w)*(s-h));
    }
    void show(Delta *p){
	cout<<"l="<<p->l<<","<<"w="<<p->w<<","<<"h="<<p->h<<" "<<endl;
	cout<<"is delta ? :"<<p->judge()<<endl;
	cout<<"girth = "<<p->girth()<<endl;
	cout<<"area = "<<p->area()<<endl;
    }
    void show(){
	cout<<"l="<<l<<","<<"w="<<w<<","<<"h="<<h<<" "<<endl;
	cout<<"is delta ? :"<<judge()<<endl;
	cout<<"girth = "<<girth()<<endl;
	cout<<"area = "<<area()<<endl;
    }
    void set(float l,float w,float h){
	this->l=l;
	this->w=w;
	this->h=h;
    }
};
#if 0
    写一个Person类
    成员变量：id,name,sex
    成员函数：显示对象的信息，show
#endif
struct Person{
    protected://受保护的，本类内和它的派生类可以访问
	      //外部不可访问
    private://只有在类内可以直接访问
    int id;
    string name,sex;
    public://类内和类外都能访问
    void init(Person *p,int id,string name,string sex){
	p->id=id;
	p->name=name;
	p->sex=sex;
    }
    void init(int id,string name,string sex){
	this->id=id;
	this->name=name;
	this->sex=sex;
    }
    void show(){
	cout<<"id :"<<id<<endl;
	cout<<"name :"<<name<<endl;
	cout<<"sex :"<<sex<<endl;
    }
    void set_name(string name){
	this->name=name;
    }
}; 
int main(){
#if 0
//    Delta d1{5,4,5};
//    d1.show(&d1);
    Delta d2;
    d2.set(3,4,5);
    d2.show();
    return 0;
#endif
#if 1
    Person d1;
    Person d2;

    d1.init(&d2,1002,"lisi","男");
    d2.show();
    cout<<endl;

    d1.init(1001,"lisi","男");
    d1.show();
    cout<<endl;

    d1.set_name("pipi");
    d1.show();
#endif
}
