#ifndef _LINK_H
#define _LINK_H
//link.h
//双向链表数据声明，函数声明
typedef struct{	    //学生
    char number[20];//帐号
    char key[20];   //密码
    int num;        //学号
    char name[20];  //姓名
    int age;        //年龄
    char sex[10];    //性别
//    float score;    //成绩
    char job[20];   //职务
    char clas1[10]; //班级
    char clas2[10]; //学生类别
    float chinese;  //语文
    float math;     //数学
    float english;  //英语
    float total;    //总分
}Data;
typedef struct s{   //学生
    Data value;
    struct s *pre;
    struct s *next;
}ST;
typedef struct{	    //教师
    char number[20];//帐号
    char key[20];   //密码
    int num;        //工号
    char name[20];  //姓名
    int age;        //年龄
    char sex[10];   //性别
    char job[20];   //职务
    char clas[10];  //任课班级
    char course[10];//教学科目
}Data1;
typedef struct s1{  //教师
    Data1 value1;
    struct s1 *pre;
    struct s1 *next;
}ST1;

extern ST *create_link(ST *head,Data d1);//尾创建(学生)
extern ST1 *create2_link(ST1 *head,Data1 d1);//尾创建(教师)
extern ST *create1_link(ST *head,Data d1);//头创建

extern ST *search_link(ST *head,char arr[]);//帐号查找(学生)
extern ST *search2_link(ST *head,int num);//学号查找(学生)
extern ST1 *search1_link(ST1 *head,char arr[]);//帐号查找(教师)
extern ST1 *search3_link(ST1 *head,int num);//工号查找(教师)
extern ST *del_link(ST *head,int num);//以学号删除(学生)
extern ST1 *del1_link(ST1 *head,int num);//以工号删除(教师)
extern ST *insert_link(ST *head,Data d1,char arr[]);//前插入
extern ST *insert1_link(ST *head,Data d1,char arr[]);//后插入

extern void printf_link_next(ST *head);//从尾打印双向链表
extern void printf_link_pre(ST *head);//从头打印双向链表(学生)
extern void printf1_link_pre(ST1 *head);//从头打印双向链表(教师)

extern ST *select_sort_link(ST *head);//选择排序

extern void free_link(ST *head);//释放双向链表

extern void save_link(ST *head);//数据保存到文件(学生)
extern void save2_link(ST *head);//数据保存到文件(学生成绩排名)
extern ST *read_link();//从文件读数据并创链表(学生)

extern void save1_link(ST1 *head);//数据保存到文件(教师)
extern ST1 *read1_link();//从文件读数据并创链表(教师)
#endif
