#include<iostream>
#include<cstring>
//link.cpp
//双向链表

using namespace std;
//数据
struct Date{
    int id;
    string name;
    int age;
//    Date() : id(0),age(0){}//无参构造函数，列表初始化
    Date() : Date(0,"--",0){}//无参委托构造函数
    Date(int i,string n,int a) : id(i),name(n),age(a){}
    //有参构造，列表初始化
    Date(const Date &d) : id(d.id),name(d.name),age(d.age){}
    //拷贝构造，列表初始化
};
//节点
struct Node{
    Date value;
    Node *pre;
    Node *next;
    Node() :pre(NULL),next(NULL){}//无参构造，列表初始化
//    Node(Date d,Node *p1,Node *p2) :value(d),pre(p1),next(p2){}
    Node(const Date &d,Node *p1,Node *p2) :value(d),pre(p1),next(p2){}
    //有参构造，列表初始化
    //const Date &d :引用但不改变d值
};
//双向链表类
class List{
    Node *head;
    Node *tail;
    int len;
    public:
    //列表初始化
    List() : head(NULL),tail(NULL),len(0){}

    //增加n个节点
    List(int n) : head(NULL),tail(NULL){
	Node *p=NULL;
	len=n<0?0:n;
	for(int i=0;i<n;i++){
	    p=new Node;
	    if(i==0)
		head=tail=p;
	    else{
		tail->next=p;
		p->pre=tail;
		tail=p;
	    }
	}
    }
    //尾创建
    void create_back(Date d){
    #if 1
	Node *p=new Node(d, NULL, NULL);
	if(head==NULL)
	    head=tail=p;
	else{
	    Node *t=head;
	    while(t->next!=NULL)
		t=t->next;
	    t->next=p;
	    p->pre=t;
	    tail=p;
	}
	len+=1;
    #endif
    #if 0
	Node *p=new Node(d,tail,NULL);
	//上一句已经包含：p->pre=tail;
	if(head==NULL)
	    head=tail=p;
	else{
	    tail->next=p;
	    tail=p;
	}
	len+=1;
    #endif
    }
    //头创建
    void create_front(Date d){
    #if 1
	Node *p=new Node{d, NULL, NULL};
	if(head==NULL)
	    head=tail=p;
	else{
	    p->next=head;
	    head->pre=p;
	    head=p;
	    Node *t=head;
	    while(t->next!=NULL)
		t=t->next;
	    tail=t;
	}
	len+=1;
    #endif
    #if 0
	Node *p=new Node(d,NULL,head);
	//上一句已经包含：p->next=head;
	if(len==0)
	    head=tail=p;
	else{
	    head->pre=p;
	    head=p;
	}
	len+=1;
    #endif
    }
    //链表拷贝构造，拷贝整条链表
    List(const List &l) : head(NULL),tail(NULL),len(0){
#if 0
	Node *p=l.head;
	for(int i=0;i<l.len;i++){
	    create_back(p->value);
	    p=p->next;
	}
#endif
#if 1
	Node *p=NULL;
	len=l.len<0?0:l.len;
	for(int i=0;i<l.len;i++){
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
	
	for(int i=0;i<l.len;i++){
	    p1->value=p2->value;//深拷贝
//	    memcpy(&(p1->value),&(p2->value),sizeof(p2->value));//浅拷贝，错误
	    p1=p1->next;
	    p2=p2->next;
	}
#endif
    }
    //从头打印双链表
    void show_front(){
	/*
	if(head){
	Node *h=head;
	while(h!=NULL){
	    cout<<h->value.id<<" "<<h->value.name<<" "<<h->value.age<<endl;
	    h=h->next;
	}
	cout<<"len="<<len<<endl;
	}
	*/
	Node *h=head;
	for(int i=0;i<len;i++){
	    cout<<h->value.id<<" "<<h->value.name<<" "<<h->value.age<<endl;
	    h=h->next;	    
	}
    }
    //从尾打印双链表
    void show_back(){
	Node *b=tail;
	for(int i=0;i<len;i++){
	    cout<<b->value.id<<" "<<b->value.name<<" "<<b->value.age<<endl;
	    b=b->pre;	    
	}	
    }
    //删除尾
    void delete_back(){
	if(head){//判断链表不是空
	    if(len==1){//链表只有1个节点
		delete head;
		head=tail=NULL;
	    }
	    else{//链表有多个节点，删除尾
		Node *p=tail->pre;
		delete tail;
		p->next=NULL;
		tail=p;
	    }
	    len--;
	}
	else
	    return ;
    }
    //删除头
    void delete_front(){
	if(head){//判断链表不是空
	    if(len==1){//链表只有1个节点
		delete head;
		head=tail=NULL;
	    }
	    else{//链表有多个节点，删除头
		Node *p=head;
		head->next->pre=NULL;
		head=head->next;
		delete p;
	    }
	    len--;
	}
	else
	    return;
    }
    //通过ID查找节点
    Node *search_by_id(int id){
	Node *p=head;
	while(p){
	    if(p->value.id==id)
		return p;
	    p=p->next;
	}
	return p;
    }
    //通过ID删除1个节点
    void delete_by_id(int id){
	Node *p=search_by_id(id);
	if(p){//判断是否找到
	    if(p->value.id==head->value.id)//删除的是头
		delete_front();
	    else if(p->value.id==tail->value.id)//删除的是尾
		delete_back();
	    else{//删数的是中间
		Node *s=p->pre;
		s->next=p->next;
		p->next->pre=s;
		delete p;
		len--;
	    }
	}
	else{
	    cout<<"ID不存在\n";
	    return;
	}
    }
    //改变节点内容
    void update_by_id(int id){
	Node *p=search_by_id(id);
	if(p){
	    cout<<"1.修改ID\n"<<"2.修改名字\n"<<"3.修改年龄\n";
	    cout<<"输入您的选择:";
	    char choose=0;
	    cin>>choose;
	    switch(choose){
		case '1':
		    cout<<"输入新ID:";
		    cin>>p->value.id;
		    break;
		case '2':
		    cout<<"输入新名字:";
		    cin>>p->value.name;
		    break;
		case '3':
		    cout<<"输入新年龄:";
		    cin>>p->value.age;
		    break;
		defalut:break;
	    }
	    cout<<"修改成功\n";
	}
	else{
	    cout<<"ID不存在\n";
	    return;
	}
    }
    //尾插入
    void insert_by_id_back(int id,Date d){
	Node *s=new Node{d,NULL,NULL};
	Node *p=search_by_id(id);
	if(p){
	    if(p==tail){//插入尾部
		s->pre=p;
		p->next=s;
		tail=s;
	    }
	    else{//插入中间
		s->next=p->next;
		s->pre=p;
		p->next->pre=s;
		p->next=s;
	    }
	    len+=1;
	}
	else{
	    cout<<"ID不存在\n";
	    return;
	}
    }
    //头插入
    void insert_by_id_front(int id,Date d){
	Node *s=new Node{d,NULL,NULL};
	Node *p=search_by_id(id);
	if(p){
	    if(p==head){
		s->next=p;
		p->pre=s;
		head=s;
	    }
	    else{
		s->next=p;
		s->pre=p->pre;
		p->pre->next=s;
		p->pre=s;
	    }
	    len+=1;
	}
	else{
	    cout<<"ID不存在\n";
	    return;
	}
    }
    //释放整个链表
    ~List(){
	while(tail!=NULL){
	    Node *s=tail;
	    tail=tail->pre;
	    cout<<"delete :"<<s->value.id<<endl;
	    delete s;
	    len--;
	}
    }
#if 0
#endif
};
int main(){
    Date d1;
    Date d2(1002,"lisi",23);
    Date d3(1003,"lisi",23);
    Date d4(1004,"lisi",23);
    Date d5(1005,"lisi",23);
    Date d6(1006,"lisi",23);
    Date d7(d6);
#if 0
    List head(5);
    head.show();
#endif
#if 1
    List head;
    head.create_back(d1);
    head.create_back(d2);
    head.create_back(d3);
    head.create_front(d4);
    head.create_front(d5);
    head.create_front(d6);
    cout<<"show_front--------------------"<<endl;
    head.show_front();
//    head.delete_front();
//    head.show();
    /*
    Node *p=head.search_by_id(10);
    if(p)
	cout<<p->value.id<<" "<<p->value.name<<" "<<p->value.age<<endl;
    else
	cout<<"no have"<<endl;
	*/
    /*
    head.delete_by_id(100);
    head.show();
    List head2(head);
    head.update_by_id(1006);
    head.show();
    head2.show();
    */
//    head.insert_by_id_back(1003,d1);
//    cout<<"show_front--------------------"<<endl;
//    head.show_front();
//    cout<<"show_back--------------------"<<endl;
//    head.show_back();
    head.insert_by_id_front(1006,d3);
    head.insert_by_id_front(1001,d4);
    head.insert_by_id_front(1002,d5);
    cout<<"show_front--------------------"<<endl;
    head.show_front();
    cout<<"show_back--------------------"<<endl;
    head.show_back();

#endif
    return 0;
}
