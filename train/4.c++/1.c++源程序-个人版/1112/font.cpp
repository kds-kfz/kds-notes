#include<iostream>
#include<cstdio>
#include<cstdlib>
#include<assert.h>
#include<cstring>
#include<unistd.h>
#include<termios.h>
#include"font.h"
using namespace std;
#if 0
    #include<termios.h>
    //函数1：
    int tcsetattr(int fd, int optinal_actions, struct termios *termios_p);
    作用：设置终端参数
    成功返回0,失败返回-1,并设置errno的值

    optinal_actions：控制修改起作用的时间，可以有以下3个参数
    TCSANOW：不等数据传输完毕就立即改变属性
    TCSADAIN：等待所有数据传输结束才改变属性
    TCSAFLUSH：清空输入输出缓存区才改变属性

    termios_p：保存要修改的参数 

    //函数2：
    int tcgetattr(int fd, struct termios *termios_p);
    fd：文件描述符
    返回的结果保存在termios结构中，
    成功返回0,失败返回非0

    struct termios{
	unsigned short c_iflag;//输入模式标志位
	unsigned short c_oflag;//输出模式标志位
	unsigned short c_cflag;//控制模式标志位
	unsigned short c_lflag;//区域模式标志位/本地模式标志/局部模式
	unsigned short c_line;//控制制line discipline
	unsigned short c_cc[NCC];//控制字符特性
    };

    c_lflag标志常量：
    ICANON：启用标准模式
    ICRNL：将输入中的回车翻译为新行(除非设置里IGNCR)
    1.ECHO：阻止键入字元的回应
    2.ICNON：常规模式标志，可以对所接收的字元在两种不同终端设备模式间来回切换
    3.ISIG：当接受到字符INTR，QUIT，SUSP，DSUSP时，产生相应的信号
    4.XCASE：不属于POSIX;LINUX下不被支持
    若同时设置了ICANON，终端只有大写。输入被转换为小写，除了有前缀字符。
    输出时，大写字符被前缀(某些系统指定的特定字符)，小写字符被转换策划嗯大写
    5.ECHO：回显输入字符
    6.ECHOE：如果同时设置了ICANON，字符ERASE擦出前一个输入字符，WERASE擦出前一个词
    7.ECHOK：如果同时设置了ICANON，字符KILL删除当前行
    8.ECHONL：如果同时设置了ICANON，回显字符NL，即使没有设置ECHO
    9.ECHOCTL
    10.ECHOPRT：不属于POSIX，如果同时设置了ICANON和IECHO，字符在删除的同时被打印
    11.ECHOKE：不属于POSIX，如果同时设置了ICANON，
    回显KILL时将删除一行中的每个字符，如同指定了ECHOE和ECHOPRT一样
    12.DEFECHO
    13.FLUSHO
    14.NOFLSH
    15.TOSTOP
    16.TTOU
    17.PENDIN
    18.IEXTEN

    #include<assert.h>
    //函数3：
    void assert(int exprssion);
    作用：如果条件返回错误，则终止程序执行
    表达式exprssion返回值为假，则向stderr打印出一条错误信息
    然后通过调用abort来终止程序运行

    //函数4：
    void *memcpy(void *dest, void *src, unsigned int count);
    用于把资源内存(src 所指向的内存区域)
    拷贝count个字节到目标内存(dest所指向的内存区域)

    //备注
    STDIIN_FILENO属于系统API接口库，其声明为int型，是一个打开文件句柄
    对应的函数主要包含：open/read/write/close
    系统文件一级提供的文件API都是以文件描述副来表示文件
    STDIN_FILENO指标准输入设备(键盘)的文件描述符
#endif

char my_getchar(){
    int c=0;
    int res=0;
    struct termios org_opts,new_opts;
    res = tcgetattr(STDIN_FILENO, &org_opts);
    assert(res == 0);
    memcpy(&new_opts,&org_opts,sizeof(new_opts));
    new_opts.c_lflag &= ~ (ICANON | ECHO | ECHOE | ECHOK | ECHONL |
	    ECHOPRT | ECHOKE | ICRNL);
    tcsetattr(STDIN_FILENO, TCSANOW, &new_opts);
    c = getchar();
    res = tcsetattr(STDIN_FILENO, TCSANOW, &org_opts);
    assert(res == 0);
    return c;
}

//光标上移x行
void fun_cursor_up(int x){
    int i=0;
//    printf("\n");//默认光标在第0列
    for(;i<x+1;i++)
//	printf("\033[1A");
	cout<<"\033[1A";
}
//光标下移x行
void fun_cursor_down(int x){
    int i=0;
//    printf("\n");//默认光标在第0列
    for(;i<x;i++)
//	printf("\n");
//	printf("\033[1B");
	cout<<"\033[1B";
}
//光标左移x列
void fun_cursor_left(int x){
    int i=0;
//    printf("\n");//默认光标在第0列
    for(;i<x;i++)
//	printf("\033[1D");
	cout<<"\033[1D";
}
//光标右移x列
void fun_cursor_right(int x){
    int i=0;
//    printf("\n");//默认光标在第0列
    for(;i<x;i++)
//	printf("\033[1C");
	cout<<"\033[1C";
}
//清空光标x个字符
void clear_cursor(int x){
    int i=0;
    for(;i<x;i++)
//	printf(" ");
	cout<<" ";
}
//设置光标位置
void set_cursor(int x,int y){
//    printf("\033[%d;%dH",y,x);
    cout<<"\033["<<y<<";"<<x<<"H";
}
//清屏
void clear_screen(){
//    printf("\033[2J");
    cout<<"\033[2J";
}
//清除光标后所在行的内容(不好用)
void clear_line(){
//    printf("\033[K");
    cout<<"\033[K";
}
//保存光标位置
void save_cursor(){
//    printf("\033[s");
    cout<<"\033[s";
}
//恢复光标位置
void recover_cursor(){
//    printf("\033[u");
    cout<<"\033[u";
}
//隐藏光标
void hide_cursor(){
//    printf("\033[?25l");
	cout<<"\033[?25l";
}
//显示光标
void show_cursor(){
//    printf("\033[?25h");
    cout<<"\033[?25h";
}
//字体闪烁
void blink_font(){
#if 1
    int i=0,j=0;
    system("clear");
    save_cursor();
    hide_cursor();
    while((j+=1000)<500000){
    i++;
    if(i%7==0)i=0;
    recover_cursor();
//    printf("\033[0m");
    cout<<"\033[0m";
    usleep(500000-j);
    recover_cursor();
//    printf("\033[5m\033[%dm\033[%dm""欢迎光临腾讯小卖部\n",40+i,31+i);
    cout<<"\033[5m\033["<<(40+i)<<"m\033["<<(31+i)<<"m欢迎光临腾讯小卖部\n";
    usleep(j);
    }
#endif
#if 0
    clear_screen();
    int i=0;

    int j=0;
    int k=1;
    int l=2;
    int n=3;
    set_cursor(33,0);
    save_cursor();
    while(i++ < 100){
	j++;
	k++;
	l++;
	n++;
	if(j%7==0&&j/10==2)
	    j=0;
	if(k%7==0&&k/10==2)
	    k=0;
	if(l%7==0&&l/10==2)
	    l=0;
	if(n%7==0&&n/10==2)
	    n=0;
	recover_cursor();
	clear_line();
//	printf("\033[%dm\033[5m""hello world color:%d count:%d\n",30+j,j,i);
	printf("\033[%dm""hello ",30+j);
	printf("\033[%dm""world ",30+k);
	printf("\033[%dm""color:%d ",30+l,l);
	printf("\033[%dm""count:%d\n",30+n,i);
	usleep(1000000);//1s=1000ms=1000000us
    }
#endif

}
//n毫秒延时
void my_ms_delay(int n){
    int i;
    for(i=n;i>0;i--)
	usleep(1000);//1ms
}
//界面选择函数,值返回首字符
char menu_choose(int n){
//    static char ch[BUFSIZ];
//    setbuf(stdoin,ch);//输入前清空缓存取,只能用1次
/*
    char choose[10]={0};
    char *p=choose;
    int i=1;//默认接收1个字符
    gets(choose);//接收空格即其它字符，不接收回车
    for(;*(p+1)!='\0';p++,i++);//统计接收字符个数
    if(i>1)//如果大于1个字符
	return 'n';//返回错误'n'
    else
	return choose[0];//输入了回车，返回'0'字符，或只输入了1个字符，返回该字符
*/
//    fflush(stdin);//清空上次键盘输入缓存取内容
//    fflush(stdout);//清空上次键盘输出缓存取内容
    char choose[50]={0};//定义足够大的字符数组，接收输入内容
    char *p=choose;
    int i=1;//默认接收1个字符
    int j=-1;//数组下标要从-1开始
    do{
    j++;
    choose[j]=getchar();
    }while(j<n&&choose[j]!='\n');//接收到回车或是字符个数大于n结束

    for(;*(p+1)!='\n';p++,i++);//统计接收字符个数
    if(i>1)//如果大于1个字符
	return 'n';//返回错误'n'
    else
	return choose[0];//输入了回车，返回'0'字符，或只输入了1个字符，返回该字符
}

//循环输入字符构成字符串函数，若第一次输入不是回车则输入有效返回1,否则返回0
int my_input1(char arr[],int n){
    int i=0;
    int j=0;
    show_cursor();//显示光标
    arr[0]=getchar();
    if(arr[0]!='\n'){
	while(arr[i]!='\n'&&i<n){//如果输入不是回车且小于n长度
	    i++;
	    arr[i]=getchar();
	}
	arr[i]='\0';//将回车替换成\0
	for(;j<n&&arr[j]!=' ';j++);//将数组里空格替换成\0
	    arr[j]='\0';
	return 0;
    }
    else
	return 1;
}
//(固定)标准输入函数
void fixed_input(char arr[],int n,int *flag){
    save_cursor();
    int count=0;
    while(*flag){
	if(count==1)
	    fun_cursor_down(2);//必须要有两行，否则出错
	*flag=my_input1(arr,n);
	if(*flag){
	count=1;
	recover_cursor();
	fun_cursor_up(1);
	}
    }
}
//(固定)标准输入函数1,返回首字母
char fixed_input1(char arr[],int n,int *flag){
    save_cursor();
    int count=0;
    int num=0;
    while(*flag){
	if(count==1)
	    fun_cursor_down(2);//必须要有两行，否则出错
	*flag=my_input1(arr,n);
	if(*flag){
	count=1;
	recover_cursor();
	fun_cursor_up(1);
	}
    }
    for(;arr[num]!='\0';num++);
    if(num==1)
	return arr[0];
    else
	return '?';
}

//输入密码函数，显示*字符
int my_input(char arr[],int n){
    int i=0;
    arr[0]=my_getchar();
    if(arr[0]!='\n'){
	while(arr[i]!='\n'&&i<n){
//	printf("*");
	cout<<"*";
	i++;
	arr[i]=my_getchar();
	}
    return 0;
    }
    else
	return 1;
}

//画边框函数
void display_cheek(int x,int y){
    int i,j;
    for(i=0;i<x;i++){
	if(i==0||i==x-1){
	    //打印前移动光标位置
//	    printf(cursor_right);
	    cout<<cursor_right;
	    for(j=0;j<x+1+(y/2-x);j++){
		if(j%2)
//		printf(blue"*");
		cout<<blue<<"*";
		else
//		printf(yellow"*");
		cout<<yellow<<"*";
//		if(j==x+(y/2-x))
//		printf(" ");
//		else
//		    printf(" ");
		cout<<" ";
	    }
//	printf("\n");
	cout<<"\n";
	}
	else{
	    //打印前移动光标位置
	    printf(cursor_right);
	    for(j=0;j<y;j++){
		if(j==0||j==y-1){
		    if(i%2)
//		    printf(red"*");
		    cout<<red<<"*";
		    else
//		    printf(green"*");
		    cout<<green<<"*";
		}
//		printf(" ");
		cout<<" ";
	    }
//	printf("\n");
	cout<<"\n";
	}
    }
}

//外边框颜色函数
void display1_cheek(int x,int y,int color){
    int i=0,j=0;
    for(;i<x+2;i++){
//	printf("\033[0m\033\[2C\033[7m\033[%dm",color);
	cout<<"\033[0m\033\[2C\033[7m\033["<<color<<"m";
	for(j=0;j<x+3+(y/2-x);j++)
//	    printf("  ");
	    cout<<"  ";
//	printf("\n");
	cout<<"\n";
    }
}
// 清屏+画边框+光标上移n行
void cursor_move(int x,int y,int n){
    system("clear");//清屏
    display_cheek(x,y);//画边框
    fun_cursor_up(n);//光标上移n行
}
