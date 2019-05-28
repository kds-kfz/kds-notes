#include"teacher.h"
#include"student.h"

//学生管理系统主界面
void menu(){
    system("clear");
    printf(cursor_right);
    printf(color_font_high"学生管理系统\n"color_end);
    printf(cursor_right);
    printf("1.学生\n");
    printf(cursor_right);
    printf("2.教师\n");
    printf(cursor_right);
    printf("3.管理员\n");
    printf(cursor_right);
    printf("4.退出\n");
    printf(cursor_right);
    printf("请输入您的选择:");
}
//教师注册函数
void te_link(){
    ST1 *head=NULL;
    int num=0;
//    int flag=0;
	int fg=0;
    head=read1_link();
	if(head==NULL){
	    printf(cursor_right);
	    printf("文件无数据,需注册\n");
	    num=2000;
	    sleep(2);
	}
//	else 
//	    flag=1;
	Data1 d1={0};
	while(1){   
	    if(head){
		ST1 *tail=head;
		while(tail->next!=NULL)
		    tail=tail->next;
		num=tail->value1.num;
		}
	system("clear");
	printf(cursor_right);
	printf("教师正在注册...\n");
	printf(cursor_right);
	printf("请输入个人信息\n");
	printf(cursor_right);
	printf("帐号\t密码\t姓名\t年龄\t性别\n");
	d1.num=++num;
	printf(cursor_right);
	scanf("%s %s %s %d %s",
		d1.number,d1.key,d1.name,&d1.age,d1.sex);
	getchar();
/*	if(flag){
	    while(head->pre!=NULL)
		head=head->pre;
	    }
	    */
	head=create2_link(head,d1);
	save1_link(head);
	
	printf(cursor_right);
	printf("是否继续注册(y or n)\n");
	printf(cursor_right);
	printf("您的选择是:");
	while(1){
	char choose=0;
/*	if(fg==0){
	    scanf("%c",&choose);
	    getchar();
	    fg=1;
	}
	else{
	    printf(cursor_right);
	    */
	    scanf("%c",&choose);
	    getchar();
//	}
	if(choose=='y')
	    break;
	else if(choose=='n'){
	    return;
	    }
	else{
	    printf(cursor_right);
	    printf("选择错误, 请再次选择\n");
	    sleep(1);
	    }
	}
    }
}
//教师注册界面函数
void te_register(){
    while(1){
    system("clear");
    printf(cursor_right);
    printf(color_font_high"教师注册界面\n"color_end);
    printf(cursor_right);
    printf("1.教师注册\n");
    printf(cursor_right);
    printf("2.退出\n");
    printf(cursor_right);
    printf("请输入您的选择:");

    int choose=0;
    scanf("%d",&choose);
    switch(choose){
	case 1:te_link();break;
	case 2:return;
	default :printf("选择错误\n");sleep(1);break;
	}
    }
}
//查看教师个人信息函数
void te_news(ST1 *te_head){
    printf(cursor_right);
    printf("帐号\t密码\t工号\t姓名\t年龄\t性别\n");
    printf(cursor_right);
    printf("%s\t%s\t%d\t%s\t%d\t%s\n",
	te_head->value1.number,te_head->value1.key,te_head->value1.num,
	te_head->value1.name,te_head->value1.age,te_head->value1.sex);
    sleep(5);
}
//教师修改密码函数
void te_key_change(ST1 *head,ST1 *te_head){
    int count=0;
    while(1){
    system("clear");
    char now_key[20]="0";
    printf(cursor_right);
    printf(color_font_high"教师修改密码界面\n"color_end);
    printf(cursor_right);
    printf("请输入旧密码:");
    scanf("%s",now_key);
    if(!strcmp(now_key,te_head->value1.key)){
	int i=0;
	for(;i<20;i++)
	    now_key[i];
	printf(cursor_right);
	printf("请输入新密码:");
	scanf("%s",now_key);
	strcpy(te_head->value1.key,now_key);
	save1_link(head);
	break;
    }
    else{
	count++;
	printf(cursor_right);
	printf("密码错误(%d),请再次输入密码\n",count);
	sleep(2);
	if(count==3)
	    break;
	continue;
	}
    }
}
//显示所有学生信息函数
void display_all_student(){
    ST *head=read_link();
    if(head==NULL){
	printf(cursor_right);
	printf("数据不存在\n");
	sleep(1);
    }
    else{
	printf(cursor_right);
	printf("学号\t姓名\t年龄\t性别\t成绩\n");
	printf_link_pre(head);
	sleep(5);
    }
}
//教师给学生录入成绩函数
void record_mark(){
    while(1){
    system("clear");
    int count=0;
    
    while(1){
    system("clear");
    int st_id=0;
    int fg=0;
    printf(cursor_right);
    printf(color_font_high"教师录成绩界面\n"color_end);
    printf(cursor_right);
    printf("请输入要录入成绩的学生学号:");
    scanf("%d",&st_id);
    
    ST *head=read_link();
    ST *st_head=head;
    if(st_head==NULL){
	printf(cursor_right);
	printf("文件不存在\n");
	sleep(2);
    }
    else{
	printf(cursor_right);
	st_head=search2_link(st_head,st_id);
	if(st_head){
	    printf("该学号学生存在\n");
	    float mark=0;
	    printf(cursor_right);
	    printf("请输入学生成绩:");
	    scanf("%f",&mark);
	    st_head->value.score=mark;
	    save_link(head);
	    
	    printf(cursor_right);
	    printf("是否继续录成绩(y or n)\n");
	    while(1){
	    char choose=0;
	    printf(cursor_right);
	    printf("您的选择是:");
	    getchar();
//	    if(fg==0){
		scanf("%c",&choose);
//		fg=1;
//		}
//	    else{
//		printf(cursor_right);
//		scanf("%c",&choose);
//	    }
	    if(choose=='y')
		break;
	    else if(choose=='n'){
		return;
		}
	    else{
		printf(cursor_right);
		printf("选择错误，请再次选择\n");
		sleep(1);
		}
	    }
	}
	else{
	    count++;
	    printf("输入学号错误(%d)，请重新输入学生学号\n",count);
	    sleep(2);
	    if(count==3)
		return;
		}
	    }
	}
    }
}
//所有学生成绩排名函数
void gpa_rank(){
    ST *head=read_link();
    if(head==NULL){
	printf(cursor_right);
	printf("数据不存在\n");
	sleep(2);
    }
    else{
	head=select_sort_link(head);
	printf_link_pre(head);
	sleep(5);
    }
}

//教师登录函数
void te_login(){
    ST1 *head=NULL;
    ST1 *te=NULL;
    ST1 *te_head=NULL;

    head=read1_link();
    char te_id[20]="0";
    char te_key[20]="0";
    int flag=1;
    int id_count=0;

    while(flag){
    int key_count=0;
    system("clear");
    printf(cursor_right);
    printf(color_font_high"教师登录界面\n"color_end);
    printf(cursor_right);
    printf("请输入教师帐号:");
    scanf("%s",te_id);
    te=search1_link(head,te_id);
    te_head=te;

    if(te!=NULL){
	printf(cursor_right);
	printf("帐号存在\n");
	sleep(1);
	while(1){
	    system("clear");
	    printf(cursor_right);
	    printf(color_font_high"教师登录界面\n"color_end);
	    printf(cursor_right);
	    printf("请输入教师密码:");
	    scanf("%s",te_key);
	    if(!strcmp(te_key,te->value1.key)){
		printf(cursor_right);
		printf("教师登录成功\n");
		sleep(1);
		flag=0;
		break;
		}
	    else{
		printf(cursor_right);
		printf("帐号与密码不匹配，请再次输入密码\n");
		key_count++;
		printf(cursor_right);
		printf("密码错误(%d)\n",key_count);
		sleep(2);
		if(key_count==3)
		    break;
		}
	    }
	}
	else{
	    id_count++;
	    printf(cursor_right);
	    printf("帐号不存在，帐号错误(%d)\n",id_count);
	    sleep(2);
	    if(id_count==3)
		return ;
	    continue;
	}
    }
    
    while(1){
    system("clear");
    printf(cursor_right);
    printf(color_font_high"教师个人信息\n"color_end);
    printf(cursor_right);
    printf("1.查看自己信息\n");
    printf(cursor_right);
    printf("2.修改密码\n");
    printf(cursor_right);
    printf("3.查看所有学生\n");
    printf(cursor_right);
    printf("4.录入成绩\n");
    printf(cursor_right);
    printf("5.学生成绩排名\n");
    printf(cursor_right);
    printf("6.退出\n");
    printf(cursor_right);
    printf("请输入您的选择:");

    int choose=0;
    scanf("%d",&choose);
    switch(choose){
	case 1:te_news(te_head);break;
	case 2:te_key_change(head,te_head);break;
	case 3:display_all_student();break;
	case 4:record_mark();break;
	case 5:gpa_rank();break;
	case 6:return;
	default :printf(cursor_right);printf("选择错误\n");break;
	}
    }
}
//教师界面函数
void teacher(){
    while(1){
    system("clear");
    printf(cursor_right);
    printf(color_font_high"教师界面\n"color_end);
    printf(cursor_right);
    printf("1.注册\n");
    printf(cursor_right);
    printf("2.登录\n");
    printf(cursor_right);
    printf("3.退出\n");
    printf(cursor_right);
    printf("请输入您的选择:");

    int choose=0;
    scanf("%d",&choose);
    switch(choose){
	case 1:te_register();break;
	case 2:te_login();break;
	case 3:return;
	default :printf(cursor_right);printf("选择错误\n");sleep(1);break;
	}
    }
}
//修改管理员信息函数
void admin_change(){
    char id[20]="0";
    char key[20]="0";
    while(1){
    system("clear");
    printf(cursor_right);
    printf(color_font_high"管理员修改信息界面\n"color_end);
    FILE *fp=fopen(".admin.txt","r+");
    char id1[20]="0";
    char key1[20]="0";
    if(fp){
	printf(cursor_right);
	printf("打开成功\n");
	sleep(1);

	fscanf(fp,"%s",id);
	fscanf(fp,"%s",key);
	fclose(fp);
	}
    
	while(1){
	FILE *fp1=fopen(".admin.txt","w");

	printf(cursor_right);
	printf("请再次输入密码:");
	getchar();
	scanf("%s",key1);
	
	if(!strcmp(key,key1)){
	    printf(cursor_right);
	    printf("请输入新帐号:");
	    scanf("%s",id1);
	    printf(cursor_right);
	    printf("请输入新密码:");
	    scanf("%s",key1);
	    
	    fprintf(fp1,"%s\n",id1);
	    fprintf(fp,"%s",key1);

	    fclose(fp1);
	    printf(cursor_right);
	    printf("数据保存成功\n");
	    sleep(2);
	    return;
	    }
	else{
	    fprintf(fp1,"%s\n",id);
	    fprintf(fp1,"%s",key);
	    printf(cursor_right);
	    printf("密码校验失败\n");
	    fclose(fp1);
	    sleep(2);
//	    continue;
	    }
	
	printf(cursor_right);
	printf("是否继续(y or n)\n");
	while(1){
	char choose=0;
	printf(cursor_right);
	printf("请输入您的选择:");
	getchar();
	scanf("%c",&choose);
	if(choose=='y')
	    break;
	else if(choose=='n'){
//	    fclose(fp1);
	    return;
	}
	else{
	    printf(cursor_right);
	    printf("选择错误\n");
	    sleep(2);
	    }
	}
    }

    }
}

//显示所有教师信息函数
void display_all_teacher(){
    ST1 *head=read1_link();
    if(head==NULL){
	printf(cursor_right);
	printf("数据不存在\n");
	sleep(1);
    }
    else{
	printf(cursor_right);
	printf("帐号\t密码\t工号\t姓名\t年龄\t性别\n");
	printf1_link_pre(head);
	sleep(5);
    }
}

//注销学生函数
void del_student(){
    while(1){
    system("clear");
    int st_num=0;
    printf(cursor_right);
    printf(color_font_high"管理员注销学生界面\n"color_end);
    printf(cursor_right);
    printf("请输入要注销学生的学号:");
    scanf("%d",&st_num);
    ST *head=read_link();

    if(head==NULL){
	printf(cursor_right);
	printf("学生数据为空\n");
	sleep(2);
	return;
    }
    head=del_link(head,st_num);
    save_link(head);
    
    printf(cursor_right);
    printf("是否继续注销学生(y or n)\n");
    while(1){
    char choose=0;
    getchar();
    printf(cursor_right);
    printf("输入您的选择:");
    scanf("%c",&choose);
    if(choose=='y')
	break;
    else if(choose=='n')
	return;
    else{
	printf(cursor_right);
	printf("选择错误\n");
	    }
	}
    }
}

//注销教师函数
void del_teacher(){
    while(1){
    system("clear");
    int te_num=0;
    printf(cursor_right);
    printf(color_font_high"管理员注销教师界面\n"color_end);
    printf(cursor_right);
    printf("请输入要注销教师的工号:");
    scanf("%d",&te_num);
    ST1 *head=read1_link();
    
    if(head==NULL){
	printf(cursor_right);
	printf("教师数据为空\n");
	sleep(2);
	return;
    }
    head=del1_link(head,te_num);
    save1_link(head);
    
    printf(cursor_right);
    printf("是否继续注销教师(y or n)\n");
    while(1){
    char choose=0;
    printf(cursor_right);
    printf("输入您的选择:");
    getchar();
    scanf("%c",&choose);
    if(choose=='y')
	break;
    else if(choose=='n')
	return;
    else{
	printf(cursor_right);
	printf("选择错误\n");
	    }
	}
    }
}

//管理员登录函数
void ma_login(){
    FILE *fp=fopen(".admin.txt","r");
    if(fp!=NULL){
	printf(cursor_right);
	printf("打开成功\n");
	sleep(1);

	char id[20]="0";
	char key[20]="0";

	fscanf(fp,"%s",id);
	fscanf(fp,"%s",key);
	fclose(fp);

	int flag=1;
	
	while(flag){
	    int count=0;
	    while(1){
	    system("clear");
	    char id1[20]="0";
	    char key1[20]="0";
	    printf(cursor_right);
	    printf(color_font_high"管理员登录界面\n"color_end);
//	    getchar();
	    printf(cursor_right);
	    printf("请输入管理员帐号:");
	    scanf("%s",id1);
	    printf(cursor_right);
	    printf("请输入管理员密码:");
	    scanf("%s",key1);

	    if(!strcmp(id,id1)&&!strcmp(key,key1)){
		printf(cursor_right);
		printf("登录成功\n");
		sleep(2);
		flag=0;
		break;
		}
	    else{
		count++;
		printf(cursor_right);
		printf("帐号与密码不匹配(%d)\n",count);
		sleep(2);
		if(count==3){
		    printf(cursor_right);
		    printf("登录失败\n");
		    sleep(1);
		    return;
		    }
		}
	    }
	}
    }
    else{
	printf(cursor_right);
	printf("打开失败\n");
	sleep(1);
    }
    //管理员登录成功后，进入管理界面
    while(1){
    system("clear");
    printf(cursor_right);
    printf(color_font_high"管理员管理界面\n"color_end);
    printf(cursor_right);
    printf("1.修改管理员信息\n");
    printf(cursor_right);
    printf("2.查看所有学生\n");
    printf(cursor_right);
    printf("3.查看所有教师\n");
    printf(cursor_right);
    printf("4.注销学生\n");
    printf(cursor_right);
    printf("5.注销教师\n");
    printf(cursor_right);
    printf("6.退出\n");
    printf(cursor_right);
    printf("请输入您的选择:");

    int choose=0;
    scanf("%d",&choose);
    switch(choose){
	case 1:admin_change();break;
	case 2:display_all_student();break;
	case 3:display_all_teacher();break;
	case 4:del_student();break;
	case 5:del_teacher();break;
	case 6:return;
	default :printf(cursor_right);printf("选择错误\n");break;
	}
    }
}
//管理员界面函数
void manager(){
    while(1){
    system("clear");
    printf(cursor_right);
    printf(color_font_high"管理员界面\n"color_end);
    printf(cursor_right);
    printf("1.登录\n");
    printf(cursor_right);
    printf("2.退出\n");
    printf(cursor_right);
    printf("请输入您的选择:");

    int choose=0;
    scanf("%d",&choose);
    switch(choose){
	case 1:ma_login();break;
	case 2:return;
	default:printf(cursor_right);printf("选择错误\n");sleep(1);break;
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
	default:printf(cursor_right);printf("选择错误\n");sleep(1);break;
	}
    }
}

