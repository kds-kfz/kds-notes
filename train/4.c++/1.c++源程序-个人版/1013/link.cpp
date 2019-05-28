#include"link.h"

//单链表尾创建
Stu *creat_link_beh(Stu *head,Stu d1){
    Stu *p=new Stu;
#if 0
    if(p)
	cout<<"ok\n";
    else
	cout<<"error\n";
#endif
	*p=d1;
	p->next=NULL;
	if(head==NULL)
	    return p;
	Stu *tail=head;
	while(tail->next!=NULL)
	    tail=tail->next;
	tail->next=p;
	return head;
}
//单链表头创建
Stu *creat_link_pre(Stu *head,Stu d1){
    Stu *p=new Stu;
    *p=d1;
    p->next=head;
    return p;
}
//单链表打印
void print_link(Stu *head){
    while(head){
	cout<<head->id<<" "<<head->name<<" "<<head->score<<endl;
	head=head->next;
    }
}
//string转换int只转数字字符
int string_int(string str){
    int num=0;
    for(int i=0;i<str.size();i++)
	if(str[i]>='0'&&str[i]<='9'){
	    num*=10;
	    num+=str[i]-'0';
	}
    return num;
}
//单链表查找
Stu *seek_link(Stu *head,int id){
    while(head){
	if(head->id==id)
	    return head;
	else
	    head=head->next;
    }
    return head;//NULL
}
//单链表头插入
Stu *insert_pre(Stu *head,Stu d1,int id){
    Stu *p=new Stu;
    *p=d1;
    Stu *t=head;
    Stu *s=p;
    //位置是头
    if(t->id==id){
	p->next=head;
	return p;
    }
    //位置是中间
    while(t!=NULL){
	if(t->id==id)
	    break;
	else{
	    s=t;
	    t=t->next;
	}
    }
    if(t){
	p->next=t;
	s->next=p;
    }
    else
	cout<<"前插入失败"<<endl;
    return head;
}
//单链表尾插入
Stu *insert_beh(Stu *head,Stu d1,int id){
    Stu *p=new Stu;
    *p=d1;
    Stu *t=head;
    t=seek_link(t,id);
    //位置是中间或结尾
    if(t){
	p->next=t->next;
	t->next=p;
    }
    else
	cout<<"尾插入失败"<<endl;
    return head;
}
//单链表删除
Stu *del_link(Stu *head,int id){
    Stu *p=head;
    Stu *s=p;
    //删除的是头
    if(p->id==id){
	head=head->next;
	delete p;
	return head;
    }
    //删除的是中间或结尾 
    while(p){
	if(p->id==id)
	    break;
	else{
	    s=p;
	    p=p->next;
	}
    }
    if(p){
	s->next=p->next;
	delete p;
	return head;
    }
    else{
	cout<<"删除失败"<<endl;
	return head;
    }
}
#if 0
//单链表倒置：1->2->3
/-------------------/
          h
      t   n
NULL<-1   2->3
/------------------/
             h
         t   n
NULL<-1<-2   3
/------------------/
                n=NULL 
		h=NULL
            t   
NULL<-1<-2<-3
#endif
//单链表倒置
Stu *reverse_link(Stu *head){
    Stu *n=NULL;
    Stu *t=NULL;
    while(head){
	n=head->next;
	head->next=t;
	t=head;
	head=n;
    }
    return t;
}

//单链表释放
void delete_link(Stu *head){
    while(head){
	Stu *s=head;
	head=head->next;
	delete s;
    }
}

//单链表选择排序
Stu *select_sort_link(Stu *head){
    Stu *news=NULL;//新链表头
    while(head){
    Stu *p=NULL;//当前
    Stu *t=NULL;//保存p前一个
    Stu *pmax=head;//默认第一个最大
    for(p=head; p->next!=NULL;p=p->next){
	if(p->next->score > pmax->score){
	    t=p;
	    pmax=p->next;//保存最大数据指针
	}
    }
    if(t==NULL){//第一个最大
	head=head->next;
    }
    else{
	t->next=pmax->next;
    }
    pmax->next=news;
    news=pmax;
    }
    return news;
}
#if 0
插入排序---数组：3->2->1->7->5  
/-----------------------------/
       news         n
NULL   3-> NULL  3->2->1->7->5
t      p
/-----------------------------/
       news                  n
NULL   2  ->3->NULL  3->2->1->7->5
t      p  
/-----------------------------/
       news                      n
NULL   1  ->2->3->NULL  3->2->1->7->5
t      p  
/-----------------------------/
       news                           n
NULL   1->2->3->7 ->NULL  3->2->1->7->5
	        t   p  
/-----------------------------/
       news                           
NULL   1->2->3->7 ->NULL  3->2->1->7->5
	     t  p  
/-----------------------------/
       news
NULL   1->2->3->5->7 ->NULL  3->2->1->7->5
	     t  p  
#endif
//单链表插入排序
Stu *insert_sort_link(Stu *head){
    Stu *news=head;//new保存第一个
    head=head->next;//从第二个开始
    news->next=NULL;
    while(head){
	Stu *n=head;//n保存第二个
	head=head->next;//head往下走
	Stu *p=news;//p保存第一个
	Stu *t=NULL;
	while(p != NULL && n->score > p->score){
	    t=p;
	    p=p->next;
	}
	if(t==NULL){
	n->next=news;
	news=n;
	}
	else{
	n->next=t->next;
	t->next=n;
	}
    }
    return news;
}


