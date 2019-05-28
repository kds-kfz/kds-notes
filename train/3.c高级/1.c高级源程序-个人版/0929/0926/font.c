#include"font.h"
#include<stdio.h>

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

int main1(){
    int choose=0;
//    system("clear");
    display_cheek(20,60);
    CU;
    
//    back_color(20,60);

    printf(cursor_right7"学 生 管 理 系 统\n\n");
//    CR1;
    printf(cursor_right1"系统版本：文件版\n\n");
//    CR1;
    printf(cursor_right9"1.学  生\n\n");
//    CR1;
    printf(cursor_right3"2.教  师\n\n");
//    CR1;
    printf(cursor_right4"3.管理员\n\n");
//    CR1;
    printf(cursor_right5"4.退  出\n\n");
//    CR1;
    printf(cursor_right6"请输入您的选择:");
//    while(1);


    scanf("%d",&choose);
    
    return 0;
}

