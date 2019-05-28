#include<iostream>
//7.cpp
using namespace std;

//多态原理(老师补充)

//纯虚构
//有纯虚函数的类叫做抽象类，抽象类不能定义对象(实例)，但可以定义指针
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

#if 0
Person p;
Ste t;
//Person &r=t;

Person *p=new Person;
//如果基类是抽象类，则此定义错误，不能定义实例，只能定义指针
//如果基类不是抽象类，正确

//下面内容讲述多态原理:
/***********************
    (pvtab)虚函数表的指针,实际上没有指针，
    但地址确实存在，这里随便取个名字叫(pvtab)
    ----------------------------------------------------------------
    Person *p=new Person;   基类虚函数表自己的地址(个人补充)
    (pvtab)虚函数表指针	    -->[Person::show|Person::isGood|~Person]
    id
    name
    ----------------------------------------------------------------
    Person *p=new Ste;	    派生类虚函数表自己的地址(个人补充)
    (pvtab)虚函数表指针	    -->[Ste::show|Person::isGood|~Ste]
    id			     show()有重写|isGood()没重写	
    name		     有自己的地址|是基类函数的地址
    继承基类的成员变量	     自己的show()|继承基类的isGood()
    ==============
    count
    自己的私有成员变量
  ***********************/
    //老师笔记
    //1.如果派生类没有堆基类的虚函数重写，派生类的虚函数表存储的是基类虚函数的地址
    //2.如果派生类里对基类的虚函数进行重写，派生类的虚函数表就会改变
    //个人补充
    //1.每个派生类虚函数表的地址空间只有一个，派生类里重写的虚函数，
    //2.会将其重写好的虚函数地址存到自己的虚函数表里，
    //其虚函数表里的虚函数地址与重写虚函数的先后顺序有关，
    //先写先存，不写则与基类虚函数表里的虚函数地址顺序一致
    //个人补充
    //注意：
    //1.虚函数表地址存在代码区
    //2.每个类虚函数表只有1个，其表里的虚函数地址可以不同
    //3.可以通过基类指针，访问派生类虚函数表里的内容，
    //打印其虚函数，此时需要多次指针强转
#endif

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
