#ifndef _FONT_H
#define _FONT_H

/**************************************************
光标，字体操作
0m：关闭属性
1m：高亮度
4m：下划线
5m：闪烁
7m：反显
8m：消影
nA：光标上移n行
nB：光标下移n行
nC：光标右移n列
nD：光标左移n列
背景颜色：40--49(黑，红，绿，黄，蓝，紫，深绿，白)
字体颜色：30--39(黑，红，绿，黄，蓝，紫，深绿，白)
格式：printf("\033[背景颜色;字体颜色m字符串\033[0m");
****************************************************/

//设置高亮度，字体颜色：蓝色
#define  color_font_high "\033[1m\033[34;1m"

//关闭所有属性
#define  color_end "\033[0m"

//设置前景颜色：黄色
#define color_font_yellow "\033[33m"

//设置背景颜色：红色
#define color_back_red "\033[41m"

//光标移动
#define cursor_up "\033[23A"
#define cursor_down "\033[1B"
#define cursor_down1 \033[1B
#define cursor_right "\033[5C"
#define cursor_right11 "\033[25C"
#define cursor_right12 "\033[20C"
#define cursor_right13 "\033[12C"
#define cursor_right14 "\033[5C"
#define cursor_right15 "\033[10C"
#define cursor_right16 "\033[3C"

//宏定义：画边框x与y坐标
#define x_seat 25
#define y_seat 86

//右移(33站位符)+高亮+字体颜色
#define cursor_right0 "\033[33C\033[1m\033[30m"//黑
#define cursor_right1 "\033[33C\033[1m\033[31m"//红
#define cursor_right2 "\033[33C\033[1m\033[32m"//绿
#define cursor_right3 "\033[33C\033[1m\033[33m"//黄
#define cursor_right4 "\033[33C\033[1m\033[34m"//蓝
#define cursor_right5 "\033[33C\033[1m\033[35m"//紫
#define cursor_right6 "\033[33C\033[1m\033[36m"//深绿
#define cursor_right7 "\033[33C\033[1m\033[37m"//白
#define cursor_right8 "\033[33C\033[1m\033[38m"
#define cursor_right9 "\033[33C\033[1m\033[39m"

#define cursor_right10 "\033[1C"

#define black "\033[1m\033[30m"
#define red "\033[1m\033[31m"
#define green "\033[1m\033[32m"
#define yellow "\033[1m\033[33m"
#define blue "\033[1m\033[34m"
#define purple "\033[1m\033[35m"
#define dark_green "\033[1m\033[36m"
#define white "\033[1m\033[37m"

#define cursor_left "\033[10D"

#define CU printf(cursor_up)
#define CD printf(cursor_down)
#define CR printf(cursor_right10)
#define CR1 printf(cursor_right1)
#define CL printf(cursor_left)

//字体函数声明

//画边框函数
extern void display_cheek(int x,int y);
//清屏+画边狂+向上移动光标
extern void cursor_move(int x,int y);
//菜单选择函数
extern char menu_choose();

#endif
