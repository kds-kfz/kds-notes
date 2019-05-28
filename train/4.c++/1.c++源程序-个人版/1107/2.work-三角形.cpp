#include<iostream>
//2.work-三角形.cpp
using namespace std;

//练习：定义一个三角形类，如果三边不能呢个构成三角形，阻止这个对象构造
class Delta{
    int d1,d2,d3;
    public:
    Delta():Delta(0,0,0){}
    Delta(int a,int b,int c) : d1(a),d2(b),d3(c){
	if(!ok())
	    throw "不能构成三角形";
	cout<<"d1="<<d1<<",d2="<<d2<<",d3="<<d3<<endl;
    }
    bool ok(){
	if(d1!=0&&d2!=0&&d3!=0)
	    return d1+d2>d3&&d1+d3>d2&&d2+d3>d1;
	else
	    return false;
    }
    ~Delta(){
	cout<<"~Delta"<<endl;
    }
};

int main(){
    Delta *p=NULL;
    cout<<"&p = "<<p<<endl;
    try{
	p=new Delta(3,4,5);
	cout<<"&p = "<<p<<endl;
	cout<<"try end"<<endl;
    }
    catch(const char *s){
	delete p;
	cout<<"&p = "<<p<<endl;
	cout<<s<<endl;
	return 0;
    }
    catch(...){
	cout<<"..."<<endl;
    }
    cout<<"main end"<<endl;
    return 0;
}
