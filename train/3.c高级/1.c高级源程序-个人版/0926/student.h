#ifndef _STUDENT_H
#define _STUDENT_H
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<string.h>
#include"link.h"


#define  cursor_right1 "\033[\tC"
#define  cursor_right "\033[\t\t\tC"
#define  color_font_high "\033[1m\033[34;1m"
#define  color_end "\033[0m"
/*
#define color_font_yellow "\033[1;33m"
#define color_back_red "\033[41m"
*/

extern void student();//学生界面函数
extern void st_register();//学生注册界面函数
extern void st_link();//学生注册函数
extern void st_login();//学生登录函数
extern void st_news(ST *st_head);//查看学生个人信息函数
extern void st_key_change(ST *head,ST *st_head);//学生修改密码函数

#endif
