#ifndef _TEACHER_H
#define _TEACHER_H
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include"link.h"

extern void teacher();//教师界面函数
extern void te_register();//教师注册界面函数
extern void te_link();//教师注册函数
extern void te_login();//教师登录函数
extern void te_news(ST1 *te_head);//查看教师个人信息函数
extern void te_key_change(ST1 *head,ST1 *te_head);//教师修改密码函数

extern void display_all_student();//显示所有学生信息函数
extern void record_mark();//教师给学生录入成绩函数
extern void gpa_rank();//所有学生成绩排名函数

extern void manager();//管理员界面函数
extern void ma_login();//管理员登录函数

extern void admin_change();//修改管理员信息函数
extern void display_all_teacher();//显示所有教师信息函数
extern void del_student();//注销学生函数
extern void del_student();//注销老师函数

extern void my_system();//学生系统函数

#endif
