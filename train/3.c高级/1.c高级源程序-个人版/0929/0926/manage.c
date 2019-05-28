#include"manage.h"
#include"student.h"
#include"teacher.h"
#include"font.h"
//manage.c

//学生管理系统主界面
void menu(){
    system("clear");
    display_cheek(20,60);
    CU;
    printf(cursor_right1"学 生 管 理 系 统\n\n");
    printf(cursor_right2"系统版本:文件版v1.2\n\n");
    printf(cursor_right3"1.学  生\n\n");
    printf(cursor_right4"2.教  师\n\n");
    printf(cursor_right5"3.管理员\n\n");
    printf(cursor_right6"4.退  出\n\n");
    printf(cursor_right8"请输入您的选择:");
}

//修改管理员信息函数
void admin_change(){
    char id[20]="0";
    char key[20]="0";
    while(1){
    system("clear"); 
    display_cheek(20,60);
    CU;
    printf(cursor_right1"管 理 员 修 改 信 息 界 面\n\n");
    FILE *fp=fopen(".admin.txt","r+");
    char id1[20]="0";
    char key1[20]="0";
    if(fp){//文件读取成功
	printf(cursor_right4"打开成功\n\n");
	sleep(1);

	fscanf(fp,"%s",id);//保存旧帐号
	fscanf(fp,"%s",key);//保存旧密码
	fclose(fp);
	}
    else{
	printf(cursor_right5"文件不存在\n\n");
	sleep(2);
	return ;//结束本函数
	}
    
	while(1){
	system("clear"); 
	display_cheek(20,60);
	CU;
	printf(cursor_right1"管 理 员 修 改 信 息 界 面\n\n");

	FILE *fp1=fopen(".admin.txt","w");

	printf(cursor_right2"请再次输入密码:");
	getchar();
	scanf("%s",key1);
	
	if(!strcmp(key,key1)){//校验旧密码成功
	    putchar(10);
	    getchar();
	    printf(cursor_right3"请输入新帐号:");
	    scanf("%s",id1);
	    putchar(10);
	    getchar();
	    printf(cursor_right6"请输入新密码:");
	    scanf("%s",key1);
	    
	    fprintf(fp1,"%s\n",id1);//向文件写入新帐号
	    fprintf(fp,"%s",key1);//向文件写入新密码

	    fclose(fp1);//关闭文件
	    putchar(10);
	    getchar();
	    printf(cursor_right4"数据保存成功\n\n");
	    sleep(2);
	    return;
	    }
	else{//旧密码校验失败
	    fprintf(fp1,"%s\n",id);
	    fprintf(fp1,"%s",key);
	    printf(cursor_right1"密码校验失败\n\n");
	    fclose(fp1);//关闭文件
	    sleep(2);
//	    continue;
	    }
	int count=0;
	printf(cursor_right5"是否继续(y or n)\n\n");
	while(1){
	char choose=0;
	printf(cursor_right3"请输入您的选择:");
	getchar();
	scanf("%c",&choose);
	if(choose=='y')//是否继续校验旧密码
	    break;
	else if(choose=='n'){
//	    fclose(fp1);
	    return;//结束本函数
	}
	else{
	    count++;
	    printf(cursor_right1"选择错误(%d)\n\n",count);
	    sleep(2);
	    if(count==2)
		return;
		}
	    }
	}
    }
}

//显示所有教师信息函数
void display_all_teacher(){
    ST1 *head=read1_link();
    if(head==NULL){
	printf(cursor_right2"数据不存在\n\n");
	sleep(1);
    }
    else{
	system("clear"); 
	display_cheek(20,60);
	CU;
	ST1 *p=head;
	int count=0;
	while(p->next!=NULL){
	    ++count;
	    p=p->next;
	}
	count+=1;
	printf(cursor_right3"所 有 教 职 工 信 息\n");
	printf(cursor_right2"教 职 工 人 数：%d人\n\n",count);
//	putchar(10);
	printf(cursor_right12"帐号\t工号\t姓名\t年龄\t性别\n\n");
	printf1_link_pre(head);
	sleep(5);
    }
}

//注销学生函数
void del_student(){
    while(1){
    system("clear");
    display_cheek(20,60);
    CU;
    int st_num=0;
    printf(cursor_right5"管理员注销学生界面\n\n");
    printf(cursor_right3"请输入要注销学生的学号:");
    scanf("%d",&st_num);
    ST *head=read_link();

    if(head==NULL){
	printf(cursor_right2"学生数据为空\n\n");
	sleep(2);
	return;
    }
    head=del_link(head,st_num);
    save_link(head);
    int count=0;
    printf(cursor_right2"是否继续注销学生(y or n)\n\n");
    while(1){
    char choose=0;
    getchar();
    printf(cursor_right6"输入您的选择:");
    scanf("%c",&choose);
    if(choose=='y')
	break;
    else if(choose=='n')
	return;
    else{
	count++;
	printf(cursor_right1"选择错误(%d)\n\n",count);
	sleep(1);
	if(count==2)
	    return;
	    }
	}
    }
}

//注销教师函数
void del_teacher(){
    while(1){
    system("clear");
    display_cheek(20,60);
    CU;
    int te_num=0;
    printf(cursor_right3"管理员注销教师界面\n\n");
    printf(cursor_right4"请输入要注销教师的工号:");
    scanf("%d",&te_num);
    ST1 *head=read1_link();
    
    if(head==NULL){
	printf(cursor_right1"教师数据为空\n\n");
	sleep(2);
	return;
    }
    head=del1_link(head,te_num);
    save1_link(head);
    
    int count=0;
    printf(cursor_right6"是否继续注销教师(y or n)\n\n");
    while(1){
    char choose=0;
    getchar();
    printf(cursor_right2"输入您的选择:");
    scanf("%c",&choose);
    if(choose=='y')
	break;
    else if(choose=='n')
	return;
    else{
	count++;
	printf(cursor_right1"选择错误(%d)\n\n",count);
	sleep(2);
	if(count==2)
	    return;
	    }
	}
    }
}

//管理员登录函数
void ma_login(){
    FILE *fp=fopen(".admin.txt","r");
    if(fp!=NULL){
	printf(cursor_right7"打开成功\n\n");
	sleep(1);

	char id[20]="0";//保存旧帐号
	char key[20]="0";//保存旧密码

	fscanf(fp,"%s",id);//从文件读取
	fscanf(fp,"%s",key);//从文件读取
	fclose(fp);

	int flag=1;
	
	while(flag){
	    int count=0;
	    while(1){
	    system("clear");
	    display_cheek(20,60);
	    CU;
	    char id1[20]="0";//保存新帐号  
	    char key1[20]="0";//保存新密码
	    printf(cursor_right2"管 理 员 登 录 界 面\n\n");
	    printf(cursor_right4"请输入管理员帐号:");
	    scanf("%s",id1);
	    putchar(10);
	    getchar();
	    printf(cursor_right3"请输入管理员密码:");
	    scanf("%s",key1);

	    putchar(10);
	    getchar();
	    if(!strcmp(id,id1)&&!strcmp(key,key1)){
		printf(cursor_right6"登录成功\n\n");
		sleep(2);
		flag=0;
		break;
		}
	    else{
		count++;
		printf(cursor_right1"帐号与密码不匹配(%d)\n\n",count);
		sleep(2);
		if(count==3){
		    printf(cursor_right5"登录失败\n\n");
		    sleep(1);
		    return;
		    }
		}
	    }
	}
    }
    else{
	printf(cursor_right1"打开失败\n\n");
	sleep(1);
    }
    //管理员登录成功后，进入管理界面
    while(1){
    system("clear");
    display_cheek(20,60);
    CU;
    printf(cursor_right1"管 理 员 管 理 界 面\n\n");
    printf(cursor_right2"1.修改管理员信息\n\n");
    printf(cursor_right3"2.查看所有学生\n\n");
    printf(cursor_right4"3.查看所有教师\n\n");
    printf(cursor_right5"4.注销学生\n\n");
    printf(cursor_right6"5.注销教师\n\n");
    printf(cursor_right2"6.退  出\n\n");
    printf(cursor_right3"请输入您的选择:");

    int choose=0;
    scanf("%d",&choose);
    switch(choose){
	case 1:admin_change();break;
	case 2:display_all_student();break;
	case 3:display_all_teacher();break;
	case 4:del_student();break;
	case 5:del_teacher();break;
	case 6:return;
	default :printf(cursor_right1"选择错误\n\n");break;
	}
    }
}
//管理员界面函数
void manager(){
    while(1){
    system("clear");
    display_cheek(20,60);
    CU;
    printf(cursor_right5"管 理 员 界 面\n\n"color_end);
    printf(cursor_right4"1.登  录\n\n");
    printf(cursor_right3"2.退  出\n\n");
    printf(cursor_right2"请输入您的选择:");

    int choose=0;
    scanf("%d",&choose);
    switch(choose){
	case 1:ma_login();break;
	case 2:return;
	default:printf(cursor_right1"选择错误\n\n");sleep(1);break;
	}
    }
}

//学生系统函数
void my_system(){
    while(1){
    menu();

    int choose=0;
    scanf("%d",&choose);
    switch(choose){
	case 1:student();break;
	case 2:teacher();break;
	case 3:manager();break;
	case 4:exit(-1);
	default :printf(cursor_right1"选择错误\n\n");sleep(1);break;
	}
    }
}


