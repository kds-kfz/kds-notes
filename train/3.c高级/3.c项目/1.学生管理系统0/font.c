#include"font.h"
#include<stdio.h>
#include<stdlib.h>

//界面选择函数
char menu_choose(){
//    static char ch[BUFSIZ];
//    setbuf(stdoin,ch);//输入前清空缓存取
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
    char choose[10]={0};
    char *p=choose;
    int i=1;//默认接收1个字符
    int j=-1;//数组下标要从-1开始
    do{
    j++;
    choose[j]=getchar();
    }while(j<10&&choose[j]!='\n');//接收到回车或是字符个数大于10结束

    for(;*(p+1)!='\n';p++,i++);//统计接收字符个数
    if(i>1)//如果大于1个字符
	return 'n';//返回错误'n'
    else
	return choose[0];//输入了回车，返回'0'字符，或只输入了1个字符，返回该字符
}

//画边框函数
void display_cheek(int x,int y){
    int i,j;
    for(i=0;i<x;i++){
	if(i==0||i==x-1){
	    CR;//打印前移动光标位置
	    for(j=0;j<x+1+(y/2-x);j++){
		if(j%2)
		printf(blue"*");
		else
		printf(yellow"*");
		printf(" ");
	    }
	printf("\n");
	}
	else{
	    CR;//打印前移动光标位置
	    for(j=0;j<y;j++){
		if(j==0||j==y-1){
		    if(i%2)
		    printf(red"*");
		    else
		    printf(green"*");
		}
		printf(" ");
	    }
	printf("\n");
	}
    }
}
// 清屏+画边框+向上移动光标
void cursor_move(int x,int y){
    system("clear");//清屏
    display_cheek(x,y);//画边框
    CU;//向上移动光标
}
