#include<iostream>
//1.throw抛出异常.cpp
using namespace std;

//c++异常处理
int Div(int a,int b){
    if(b==0){
#if 0
	cout<<"错误"<<endl;
	return 0;
#endif
	throw "除数是0";
    }
    return a/b;
}

class Base{
    int n;
    public:
    Base(int i):n(i){
	if(n<0)
	    throw "Base是负数";
	cout<<"Base = "<<n<<endl;
    }
//    ~Base()=delete;//析构不要写成delete
    ~Base(){
	//原则上，在析构函数里不要抛出异常
	//throw "析构抛出异常";
	cout<<"~Base"<<endl;
    }

};

int main(){
    Base *p=NULL;//delete 空指针不会有错，野指针会错误
    try{
	p=new Base(1);//只在try作用域内,出了该作用域不会析构
//	throw 0;//只要抛出异常，当前作用域下的代码不会再执行
//	Div(2,0);
//	Div(2,1);
//	throw 0;
	cout<<"end try"<<endl;
    }
#if 1
    //如果没有捕获const char *,即Dir抛出的异常异常字符串，则终端处理，段错误
    catch(const char *s){//捕获
	cout<<"捕获const char * = "<<s<<endl;
	delete p;//正确,此时p是空指针
	return 0;
    }
#endif
    catch(int a){
	cout<<"a = "<<a<<endl;
    }
    catch(...){//能捕获任何异常，必须写在try语句最后,否则错误
	cout<<"..."<<endl;//没有实例接收，但不知道是什么异常
    }
    /*
    catch(double d){//错误
	cout<<"d = "<<d<<endl;
    }
    */
    cout<<"main end"<<endl;
    return 0;
}
