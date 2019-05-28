#include<iostream>
//3.cpp
using namespace std;
#if 0
练习1:Stu,Ste
用一个数组[8], 储存Stu和Ste的对象
Person *arr[8];
arr[i]=new Stu;
if(arr[i]->isGood())
    arr[i]->show();
练习2:
基类：Person
      id,name,Person *next
      static Person *head
      virtual void show()
      virtual bool is Good()
派生：Ste
      论文数量count
      void show()
      isGood(){
      论文数量大于3是优秀
      }
派生：Stu
      score
      void show()
      isGood(){
      分数大于90是优秀
      }
Person *head
new Stu;
new Ste;
#endif

class Person{
    protected:
    int id;
    string name;
    public:
    Person *next;
    static Person *head;
    Person() : Person(0,"--",NULL){}
    Person(int i,string n,Person *s) : id(i),name(n),next(s){}
    virtual void show(){
	cout<<id<<" "<<name<<" ";
    }
    virtual bool isGood(){
	return true;
    }
    Person *get_head(){
	return head;
    }
//    friend void show_link();
};
//定义一个静态头，初始化为空
Person *Person :: head=NULL;

class Stu : public Person{
    int score;
    public:
//    using Person::next;
    Stu() : Person(0,"--",NULL),score(0){
	if(head==NULL)
	    head=this;
	else{
	    //注意
	    //Person类外定义的tail，不能访问Person中私有next
	    //即使using Person::next,也不可以
	    Person *tail=head;
	    while(tail->next)
		tail=tail->next;
	    tail->next=this;
	}
    }
    Stu(int i,string n,int num) : Person(i,n,NULL),score(num){
	if(head==NULL)
	    head=this;
	else{
	    Person *tail=head;
	    while(tail->next)
		tail=tail->next;
	    tail->next=this;
	}
    }
    bool isGood(){
	return score>90;
    }
    void show(){
	Person::show();
	cout<<score<<endl;
    }
    ~Stu(){
	cout<<"delete :"<<this->id<<endl;
	if(this==head)
	    head=head->next;
	else{
	    Person *tail=head;
	    while(tail->next!=this)
		tail=tail->next;
	    tail->next=this->next;
	}
    }
};

class Ste : public Person{
    int count;
    public:
//    using Person::next;
    Ste() : Person(0,"--",NULL),count(0){
	if(head==NULL)
	    head=this;
	else{
	    Person *tail=head;
	    while(tail->next)
		tail=tail->next;
	    tail->next=this;
	}
    }
    Ste(int i,string n,int num) : Person(i,n,NULL),count(num){
	if(head==NULL)
	    head=this;
	else{
	    Person *tail=head;
	    while(tail->next)
		tail=tail->next;
	    tail->next=this;
	}
    }
    bool isGood(){
	return count>3;
    }
    void show(){
	Person::show();
	cout<<count<<endl;
    }
    ~Ste(){
	cout<<"delete :"<<this->id<<endl;
	if(this==head)
	    head=head->next;
	else{
	    Person *tail=head;
	    while(tail->next!=this)
		tail=tail->next;
	    tail->next=this->next;
	}
    }
};

void show_link(){
    //Person *p=head;//错误，没有head声明
    //即使在全局有声明static Person *head，
    //属于基类外声明,与类内的不是同一个head
    Person *p=Person::head;
    while(p){
	p->show();
	p=p->next;
	// == p=p->Person::next;
    }
}

void show_good(){
    Person *p=Person::head;
    while(p){
	if(p->isGood())
	p->show();
    p=p->next;
    }
}

int main(){
    #if 1
    //练习2
    Stu s1(1001,"lisi",89);
    Ste t1(2001,"Red",2);
    Stu s2(1002,"lisi",90);
    Ste t2(2002,"Red",3);
    Stu s3(1003,"lisi",91);
    Ste t3(2003,"Red",4);
    show_link();
    cout<<"\n";
    cout<<"优秀名单:"<<endl;
    show_good();
    #endif
    return 0;
}
