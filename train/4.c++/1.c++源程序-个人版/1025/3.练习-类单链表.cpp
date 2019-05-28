#include<iostream>
//3.cpp
using namespace std;
#if 0
练习:
定义一个学生类Stu
1.每定义一个学生对象，让它的id，从1001自动增加
2.每个学生都是一个链表的节点
定义1个静态的头节点
3.只有1个学生类Stu,不能有其它辅助类型
#endif

class Stu{
    int id;
    string name;
    int age;
    Stu *next;
    public:
    static Stu *head;
    static int count;
    /*
    Stu() : Stu(0,"--",0){}
    Stu(int i,string n,int a) : id(i),name(n),age(a){}
    */
//    Stu() : id(count++),name("--"),age(0),next(NULL){}
    Stu(int i,string n,int a,Stu *s) : id(i),name(n),age(a),next(s){}
    Stu(string n,int a) : Stu(count++,n,a,NULL){
	//单链表尾创建
//	Stu *p=new Stu{count++,n,a,NULL};
	if(head==NULL){
	    head=this;
	}
	else{
	    Stu *tail=head;
	    while(tail->next!=NULL)
		tail=tail->next;
	    tail->next=this;
	}
    }
    Stu() : Stu(count++,"--",0,NULL){
	//单链表尾创建
//	Stu *p=new Stu{count++,"--",0,NULL};
	if(head==NULL){
	    head=this;
	}
	else{
	    Stu *tail=head;
	    while(tail->next!=NULL)
		tail=tail->next;
	    tail->next=this;
	}
    }
    static void show(){
	Stu *h=head;
	while(h){
	    cout<<h->id<<" "<<h->name<<" "<<h->age<<endl;
	    h=h->next;
	}
    }

    /*
    void free_link(){
	if(head){
	    Stu *h=head;
	    while(h){
		Stu *s=h;
		h=h->next;
		delete s;
	    }
	}
    }
    */
    ~Stu(){
	/*
	while(head){
	    Stu *s=head;
	    head=head->next;
	    cout<<"delete :"<<s->id<<endl;
	    delete s;
	}
	*/
	cout<<"delete :"<<this->id<<endl;
	if(this==head)
	    head=head->next;
	else{
	    Stu *tail=head;
	    while(tail->next!=this)
		tail=tail->next;
	    tail->next=this->next;
	}
//	delete this;	
    }
};

Stu *Stu::head=NULL;//初始化头为空
int Stu::count=1000;//初始化为1000

int main(){
    Stu *p=new Stu;
    Stu s2;
    Stu s3;
    Stu s4("lisi",34);
    Stu s5("lisi",45);
    Stu s6("lisi",22);
    s5.show();
    delete p;
//    s1.free_link();
    return 0;
}

