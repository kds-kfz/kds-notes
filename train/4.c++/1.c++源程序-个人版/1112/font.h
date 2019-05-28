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
y;xH：设置光标位置
2J：清屏
K：清除从光标到行尾的内容
s：保存光标位置
u：恢复光标位置
?25l：隐藏光标
?25h：显示光标
背景颜色：40--49(黑，红，绿，黄，蓝，紫，深绿，白)
字体颜色：30--39(黑，红，绿，黄，蓝，紫，深绿，白)
格式：printf("\033[背景颜色;字体颜色m字符串\033[0m");
****************************************************/
//光标移动
#define cursor_up "\033[21A"
#define cursor_down "\033[4B"
#define cursor_right "\033[2C"
#define cursor_left "\033[10D"

#define cursor_up1 "\033[24A"
#define cursor_right10 "\033[0m\033[2C\033[7m\033[33m"

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

//右移(33站位符)+高亮+字体颜色,个人信息光标移动
#define message_cursor0 "\033[20C\033[1m\033[30m"//黑
#define message_cursor1 "\033[20C\033[1m\033[31m"//红
#define message_cursor2 "\033[20C\033[1m\033[32m"//绿
#define message_cursor3 "\033[20C\033[1m\033[33m"//黄
#define message_cursor4 "\033[56C\033[1m\033[34m"//蓝
#define message_cursor5 "\033[56C\033[1m\033[35m"//紫
#define message_cursor6 "\033[20C\033[1m\033[36m"//深绿
#define message_cursor7 "\033[20C\033[1m\033[37m"//白
#define message_cursor8 "\033[10C\033[1m\033[38m"
#define message_cursor9 "\033[10C\033[1m\033[39m"
#define cursor_up2 "\033[14A"

//字体颜色
#define black "\033[1m\033[30m"
#define red "\033[1m\033[31m"
#define green "\033[1m\033[32m"
#define yellow "\033[1m\033[33m"
#define blue "\033[1m\033[34m"
#define purple "\033[1m\033[35m"
#define dark_green "\033[1m\033[36m"
#define white "\033[1m\033[37m"


//字体函数声明

//画边框函数
extern void display_cheek(int x,int y);
//外边框颜色函数
extern void display1_cheek(int x,int y,int color);
//清屏+画边框+光标上移n行
extern void cursor_move(int x,int y,int n);
//菜单选择函数,返回输入数组首字母，也可用于接收回车
extern char menu_choose(int n);
//设置键盘输入取消回显
extern char my_getchar();
//输入密码函数打印"*"
extern int my_input(char arr[],int n);
//循环输入字符构成字符串函数，若第一个输入的字符是回车，则输入无效返回1,否则返回0
extern int my_input1(char arr[],int n);
//(固定)标准输入函数，若输入无效则回到原来光标位置继续输入，最后有效数组内无回车与空格
//参数1：数组首地址，参数2：数组长度，参数3：有效数组标志位，默认为1
extern void fixed_input(char arr[],int n,int *flag);
//(固定)标准输入函数1，返回输入首字母，可用于界面选择或继续选择界面
extern char fixed_input1(char arr[],int n,int *flag);

//光标上移x行
extern void fun_cursor_up(int x);
//光标下移x行
extern void fun_cursor_down(int x);
//光标左移x列
extern void fun_cursor_left(int x);
//光标右移x列
extern void fun_cursor_right(int x);
//清空光标x个字符
extern void clear_cursor(int x);
//设置光标位置
extern void set_cursor(int x,int y);
//清屏
extern void clear_screen();
//清除光标后所在行的内容(不好用)
extern void clear_line();
//保存光标位置
extern void save_cursor();
//恢复光标位置
extern void recover_cursor();
//隐藏光标
extern void hide_cursor();
//显示光标
extern void show_cursor();
//字体闪烁
extern void blink_font();
//n毫秒延时
extern void my_ms_delay(int n);

#endif
