#include<stdio.h>
#include<string.h>
//4.c
//文件操作,学习1

void fun2(){
    char id[20]="0";
    char pa[20]="0";
    printf("输入帐号:");
    scanf("%s",id);
    printf("输入密码:");
    scanf("%s",pa);
    FILE *fp=fopen("1.txt","w");
    fprintf(fp,"%s\n",id);
    fprintf(fp,"%s\n",pa);
    fclose(fp);
    printf("注册成功\n");
}

void fun3(){
    FILE *fp=fopen("1.txt","r");
    if(fp!=NULL){
	printf("打开成功\n");
	//移动文件指针到末尾
	fseek(fp,0,SEEK_END);
	long int t=ftell(fp);
	if(t==0){
	    printf("没有数据\n");
	    return ;
	}
	else
	    fseek(fp,0,SEEK_SET);
	char id[20]="0";
	char pa[20]="0";
	fscanf(fp,"%s",id);
	fscanf(fp,"%s",pa);
	
	char id1[20]="0";
	char pa1[20]="0";

	printf("输入帐号:");
	scanf("%s",id1);
	printf("输入密码:");
	scanf("%s",pa1);

	if(!strcmp(id,id1)&&!strcmp(pa,pa1))
	    printf("登录成功\n");
	else
	    printf("帐号或密码错误\n");
    }
}

void fun(){
    printf("1：注册\n");
    printf("2：登录\n");
    printf("3：退出\n");
    printf("输入选择\n");
    int n=0;
    scanf("%d",&n);
    switch(n){
	case 1:fun2();break;
	case 2:fun3();break;
	case 3:return ;
	default :printf("choose error\n");break;
    }

}
int main(){
    fun();
    return 0;
}
