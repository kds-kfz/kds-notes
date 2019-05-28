#include<iostream>
//3.my_list.cpp
using namespace std;

template<typename T>
class List{
    struct Node{
	T value;
	Node *pre;
	Node *next;
	Node():pre(NULL),next(NULL),value(){}
	Node(T v,Node *p,Node *n):value(v),pre(p),next(n){}
    };
    Node *head;
    Node *tail;
    int len;
    public:
    class Iterator{
	Node *pi;
	public:
	Iterator() : pi(nullptr){}
	Iterator(Node *p) : pi(p){}
	Iterator &operator++(){
	    pi=pi->next;
	    return *this;
	}
	Iterator operator++(int){
	    Iterator t(pi);
	    pi=pi->next;
	    return t;
	}
	Iterator &operator--(){
	    pi=pi->pre;
	    return *this;
	}
	Iterator operator--(int){
	    Iterator t(pi);
	    pi=pi->pre;
	    return t;
	}
	bool operator==(const Iterator &it){
	    return pi==it.pi;
	}
	bool operator!=(const Iterator &it){
	    return pi!=it.pi;
	}
	T &operator*(){//解引用
	    return pi->value;
	}
	T *operator->(){
	    return &pi->value;
	}
	/*//std_list中没有以下运算符，可以不需要重载 
	+=
	-=
	+
	-
	 */
    };
//    List() : head(NULL),tail(NULL),Node(),len(0){
    List() : len(0){
	head=tail=new Node{};
    }
    List(int n) : head(NULL),tail(NULL){
	Node *p=NULL;
	len=n>0?n:0;
	p=new Node{};
	head=tail=p;
	for(int i=1;i<len;i++){
	    tail->next=p;
	    p->pre=tail;
	    tail=p;
	}
    }
    List(int n,const T &v){
    #if 0
	//个人版
	Node *p=NULL;
	len=n>0?n:0;
	for(int i=0;i<len;i++){
	    p=new Node;
	    p->value=v;
	    if(i==0)
		head=tail=p;
	    else{
		tail->next=p;
		p->pre=tail;
		tail=p;
	    }
	}
    #endif
    #if 1
	//教师版
	len=n>0?n:0;
	head=tail=new Node{v,NULL,NULL};
	for(int i=0;i<len-1;i++){
	    tail->next=new Node{v,tail,NULL};
	    tail=tail->next;
	}
    #endif
    }
    List(const List &l){//=delete
    #if 0
	//个人版
	Node *p=NULL;
	len=l.len>0?l.len:0;
	for(int i=0;i<len;i++){
	    p=new Node;
	    if(i==0)
		head=tail=p;
	    else{
		tail->next=p;
		p->pre=tail;
		tail=p;
	    }
	}
	Node *p1=head;
	Node *p2=l.head;
	for(int i=0;i<len;i++){
	    p1->value=p2->value;
	    p1=p1->next;
	    p2=p2->next;
	}
    #endif
    #if 1
	//教师版
	len=l.len>0?l.len:0;
	Node *p=l.head;
	head=tail=new Node{p->value,NULL,NULL};
	p=p->next;
	while(p){
	    tail->next=new Node{p->value,tail,NULL};
	    tail=tail->next;
	    p=p->next;
	}
    #endif
    }
    ~List(){
	Node *h=head;
	while(head){
	    h=head;
	    cout<<"delete :"<<h->value<<endl;
	    delete h;
	    head=head->next;
	}
    }
    void show(){
	Node *h=head;
	for(int i=0;i<len;i++){
	    cout<<h->value<<" ";
	    h=h->next;
	}
	cout<<endl;
    }
    int size(){
	return len;
    }
    bool empty(){
	return len==0;
    }
    void push_back(const T &v){
    #if 0
	//个人版
	if(len){
	    Node *p=new Node;
	    p->value=v;
	    Node *t=head;
	    while(t->next!=NULL)
		t=t->next;
	    t->next=p;
	    p->pre=t;
	    tail=p;
	}
	else
	    head->value=v; 
	len+=1;
    #endif
    #if 1
	//教师版
	if(len){
	    tail->next=new Node{v,tail,NULL};
	    tail=tail->next;
	}
	else
	    head->value=v;
	len+=1;
    #endif
    }
    void push_front(const T &v){
    #if 0
	//个人版
	Node *p=new Node;
	p->value=v;
	if(len){
	    p->next=head;
	    head=p;
	}
	else
	    head=tail=p; 
	len+=1;
    #endif
    #if 1
	//教师版
	if(len!=0){
	    head->pre=new Node{v,NULL,head};
	    head=head->pre;
	}
	else
	    head->value=v;
	len+=1;
    #endif
    }
    void pop_back(){
    #if 0
	//个人版
	if(len>1){
	Node *p=tail->pre;
	p->next=NULL;
	delete tail;
	tail=p;
	len-=1;
	}
	else if(len==1){
	    delete head;
	    head=tail=NULL;
	    len-=1;
	}
	else
	    return;
    #endif
    #if 1
	//教师版
	if(len>1){
	    tail->pre->next=NULL;
	    Node *p=tail;
	    tail=tail->pre;
	    delete p;
	    len-=1;
	}
	else if(len==1)
	    len=0;
    #endif
    }
    void pop_front(){
    #if 0
	//个人版
	if(len>1){
	Node *h=head;
	head=head->next;
	head->pre=NULL;
	delete h;
	len-=1;
	}
	else if(len==1){
	    delete head;
	    head=tail=NULL;
	    len-=1;
	}
	else
	    return;
    #endif
    #if 1
	//教师版
	if(len>1){
	    Node *p=head;
	    head=head->next;
	    head->pre=NULL;
	    delete p;
	    len-=1;
	}
	else if(len==1)
	    len=0;
    #endif
    }
    T &front(){
	return head->value;
    }
    T &back(){
	return tail->value;
    }
    void remove(const T &v){
    #if 0
	//个人版
	Node *p=head;
	if(len==0)
	    return;
	while(p){
	    if(p->value==v){
		if(len==1)
		    len=0;
		else if(p==head)
		    pop_front();
		else if(p==tail)
		    pop_back();
		else{
		    p->next->pre=p->pre;
		    p->pre->next=p->next;
		    Node *t=p->pre;
		    delete p;
		    len-=1;
		    p=t;
		}
	    }
	    p=p->next;
	}
    #endif
    #if 1
	//教师版
	Node *p=head;
	if(len==0)
	    return;
	while(p){
	    if(p->value==v){
		if(len==1){
		    len=0;
		    return;
		}
		else if(p==head){
		    pop_front();
		    p=head;
		}
		else if(p==tail){
		    pop_back();
		    p=NULL;
		}
		else{
		    p->next->pre=p->pre;
		    p->pre->next=p->next;
		    Node *t=p->next;
		    delete p;
		    len-=1;
		    p=t;
		}
	    }
	    else
		p=p->next;
	}
    #endif
    }
    Iterator begin(){
	return Iterator(head);
    }
    Iterator end(){
	return Iterator(tail->next);
    }
};

int main(){
    List <string>l1(5,"hello");
    List <string>l2(l1);
//    l2.show();
    List <string>l3;
    List <int> l4;
    l3.push_back("Red");
    l3.push_back("Blue");
    l3.push_back("hello");
//    l1.show();
//    l3.show();
    l4.push_back(34);
    l4.push_back(34);
    l4.push_back(54);
    l4.push_back(34);
    l4.push_front(-3);
    l4.push_front(34);
    l4.push_front(34);
    l4.pop_back();
    l4.show();
    l4.remove(34);
    l4.remove(-3);
    l4.remove(54);
    l4.show();
    /*
    for(auto m:l4)
	cout<<m<<" ";
    cout<<endl;
    for(List<int>::Iterator it=l4.begin();it!=l4.end();it++)
	cout<<*it<<" ";
    cout<<endl;
    */
    return 0;
}
