#include<iostream>
//6.cpp
using namespace std;

//纯虚构
//有纯虚函数的类叫做抽象类，抽象类不能定义对象(实例)，单可以定义指针
class Person{
    protected:
    int id;
    string name;
    public:
    Person() : Person(0,"--"){}
    Person(int i,string n) : id(i),name(n){}
    
    //纯虚函数，没有函数体
    virtual void show()=0;
    virtual bool isGood()=0;
    virtual ~Person(){
	cout<<"~Person"<<endl;
    }
};

//当基类是抽象类，派生如果要定义对象必须对基类的虚构函数进行重写
class Stu : public Person{
    int score;
    public:
    Stu() : Person(0,"--"),score(0){}
    Stu(int i,string n,int num) : Person(i,n),score(num){}
    bool isGood(){
	return score>90;
    }
    void show(){
	cout<<id<<" "<<name<<" "<<score<<endl;
    }
    ~Stu(){
	cout<<"~Stu"<<endl;
    }
};

class Ste : public Person{
    int count;
    public:
    Ste() : Person(0,"--"),count(0){}
    Ste(int i,string n,int num) : Person(i,n),count(num){}
    bool isGood(){
	return count>3;
    }
    void show(){
	cout<<id<<" "<<name<<" "<<count<<endl;
    }
    ~Ste(){
	cout<<"~Ste"<<endl;
    }
};

int main(){
    #if 1
    //练习1
//    Person p;//错误，抽象的类不能定义实例
//    Ste t1;//正确
//    Person *p;//正确，
//    Person *p=new Person;//错误
//    Stu *p=new Stu;//正确

    Person *arr[10]={NULL};
    for(int i=0,j=0;i<10;i++){
	if(i>4)
	    arr[i]=new Ste(2000+j,"Red",j++);
	else
	    arr[i]=new Stu(1000+i,"lisi",88+i);
    }
    cout<<"优秀名单:"<<endl;
    /*
    for(int i=0;i<10;i++){
	if(arr[i]->isGood())
	    arr[i]->show();
    }
    */
    for(auto m:arr)
	if(m->isGood())
	    m->show();
    #endif
    return 0;
}
