#include"link.h"
//main.cpp
//作业
//使用new和delete创建删除链表：实现增删查改

int main(){
#if 0
    Stu d1={0};
    Stu *p=&d1;
    Stu *head=NULL;
    int n=0;
    cout<<"请输入学生人数:";
    cin>>n;
    cout<<"输入中..."<<endl;
	
    cin>>p->id>>p->name>>p->score;
    head=creat_link_beh(head,d1);

    cout<<"打印链表"<<endl;
    print_link(head);

    cout<<"输入中..."<<endl;
    cin>>p->id>>p->name>>p->score;
    head=creat_link_beh(head,d1);

    cout<<"打印链表"<<endl;
    print_link(head);
#endif
#if 0
    Stu *head=NULL;
    Stu d1={1001,"lisi",90};
    Stu d2={1002,"lisi",80};
    Stu d3={1003,"lisi",70};
    Stu d4={1004,"lisi",60};
    Stu d5={1005,"lisi",50};
    Stu d6={1006,"lisi",40};
    //尾创建链表
    head=creat_link_beh(head,d1);
    head=creat_link_beh(head,d2);
    head=creat_link_beh(head,d3);
    //头创建链表
//    head=creat_link_pre(head,d1);
//    head=creat_link_pre(head,d2);
//    head=creat_link_pre(head,d3);
    //打印链表
    print_link(head);
    //查找链表
    Stu *p=head;
    p=seek_link(p,100);
    if(p)
	cout<<p->id<<" "<<p->name<<" "<<p->score<<endl;
    else
	cout<<"id 不存在"<<endl;
    //删除链表
//    head=del_link(head,1002);
//    print_link(head);
    //链表尾插入
//    head=insert_beh(head,d4,100);
//    head=insert_beh(head,d4,1003);
//    print_link(head);
    //链表头插入
    head=insert_beh(head,d4,100);
//    print_link(head);
    //选择排序
//    head=select_sort_link(head);
    //插入排序
//    head=insert_sort_link(head);
//    print_link(head);
    //链表倒置
//    head=reverse_link(head);
//    print_link(head);
    //链表倒置
    //释放链表
    delete_link(head);
    print_link(head);
#endif
    string str("123e456r7");
    int num=string_int(str);
    cout<<num<<endl;
    return 0;
}
