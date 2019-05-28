#include"teacher.h"
#include"student.h"
#include"font.h"
//teacher.c

//教师注册函数
void te_link(){
    ST1 *head=NULL;
    int num=0;
//    int flag=0;
	int fg=0;
    head=read1_link();
	if(head==NULL){
	    printf(cursor_right1"文件无数据,需注册\n\n");
	    num=2000;
	    sleep(2);
	}
//	else 
//	    flag=1;
	Data1 d1={0};
	Data1 *p=&d1;
	while(1){   
	    int count=0;
	    if(head){//找教师链表最后一个，获取工号
		ST1 *tail=head;
		while(tail->next!=NULL)
		    tail=tail->next;
		num=tail->value1.num;
		}
	system("clear");
	display_cheek(23,86);
	CU;
	printf(cursor_right4"教师正在注册...\n\n");
	printf(cursor_right3"请输入个人信息\n\n");
	printf(cursor_right12"帐号\t密码\t姓名\t年龄\t性别\n\n");
	d1.num=++num;//工号自动
	strcpy(d1.job,"无");//职务默认无
	printf(cursor_right12);
	scanf("%s %s %s %d %s",
		d1.number,d1.key,d1.name,&d1.age,d1.sex);
	getchar();
/*	if(flag){
	    while(head->pre!=NULL)
		head=head->pre;
	    }
	    */
	//此时需要判断教师帐号是否重复
	ST1 *repeat=head;
	repeat=search1_link(repeat,p->number);
	if(repeat){
	    printf(cursor_right1"教师帐号出现重复，注册失败\n\n");
	    sleep(2);
	}
	else{
	    head=create2_link(head,d1);
	    printf("%s\n",head->value1.job);
	}
	save1_link(head);
	
	printf(cursor_right5"是否继续注册(y or n)\n\n");
	while(1){
	char choose=0;
	printf(cursor_right6"您的选择是:");
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
	    count++;
	    printf(cursor_right1"选择错误(%d), 请再次选择\n\n",count);
	    sleep(2);
	    if(count==2)
		return;
	    }
	}
    }
}
//显示教师注册界面函数
void te_register(){
    while(1){
    system("clear");
    display_cheek(23,86);
    CU;
    printf(cursor_right6"教 师 注 册 界 面\n\n");
    printf(cursor_right5"1.教师注册\n\n");
    printf(cursor_right4"2.退  出\n\n");
    printf(cursor_right3"请输入您的选择:");

    int choose=0;
    scanf("%d",&choose);
    switch(choose){
	case 1:te_link();break;
	case 2:return;
	default :printf(cursor_right1"选择错误\n\n");sleep(2);getchar();return;
	}
    }
}
//教师查看自己信息函数
void te_news(ST1 *te_head){
    system("clear");
    display_cheek(23,86);
    CU;
    putchar(012);
    printf(cursor_right6"教 师 个 人 信 息\n\n");
    printf(cursor_right12"帐号\t密码\t工号\t姓名\t年龄\t性别\t职务\n\n");
    printf(cursor_right12"%s\t%s\t%d\t%s\t%d\t%s\t%s\n",
	te_head->value1.number,te_head->value1.key,te_head->value1.num,
	te_head->value1.name,te_head->value1.age,te_head->value1.sex,
	te_head->value1.job);
    sleep(5);
}
//教师修改密码函数
void te_key_change(ST1 *head,ST1 *te_head){
    int count=0;
    while(1){
    system("clear");
    display_cheek(23,86);
    CU;
    char now_key[20]="0";
    printf(cursor_right2"教 师 修 改 密 码 界 面\n\n");
    printf(cursor_right5"请输入旧密码:");
    scanf("%s",now_key);
    if(!strcmp(now_key,te_head->value1.key)){
	int i=0;
	for(;i<20;i++)
	    now_key[i];
	printf(cursor_right6"请输入新密码:");
	scanf("%s",now_key);
	strcpy(te_head->value1.key,now_key);
	save1_link(head);
	break;
    }
    else{
	count++;
	printf(cursor_right1"密码错误(%d),请再次输入密码\n\n",count);
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
	system("clear");
	display_cheek(23,86);
	CU;
	int count=0;
	ST *p=head;
	while(p->next!=NULL){
	    count++;
	    p=p->next;
	}
	count+=1;//把最后一个学生加入
	printf(cursor_right4"所 有 学 生 信 息\n");
	printf(cursor_right6"全 班 人 数：%d人\n\n",count);
	printf(cursor_right13"帐号\t学号\t姓名\t年龄\t性别\t成绩\t班级\t职务\t类别\n\n");
	printf_link_pre(head);
	sleep(5);
    }
}
//教师给学生录入成绩函数
void record_mark(){
    while(1){
    system("clear");
    display_cheek(23,86);
    CU;
    int count=0;
    
    while(1){
    system("clear");
    display_cheek(23,86);
    CU;
    int st_id=0;
    int fg=0;
    printf(cursor_right4"教 师 录 成 绩 与 职 务 界 面\n\n");
    printf(cursor_right2"请输入要录入成绩的学生学号:");
    scanf("%d",&st_id);
    
    ST *head=read_link();
    ST *st_head=head;
    if(st_head==NULL){
	printf(cursor_right1"文件不存在\n\n");
	sleep(2);
    }
    else{
	st_head=search2_link(st_head,st_id);
	if(st_head){
	    printf(cursor_right3"该学号学生存在\n\n");
	    float mark=0;
	    char clas1[10]="0";
	    char clas2[10]="0";
	    char job[20]="0";
	    printf(cursor_right2"学生:%s 班级:%s 职务:%s 类别:%s\n",
		    st_head->value.name,st_head->value.clas1,
		    st_head->value.job,st_head->value.clas2);
	    printf(cursor_right5"请录入学生成绩:");
	    scanf("%f",&mark);
	    st_head->value.score=mark;
/*
	    printf(cursor_right5"请录入学生班级:");
	    scanf("%s",clas1);
	    strcpy(st_head->value.clas1,clas1);
	    printf(cursor_right5"请录入学生职务:");
	    scanf("%s",job);
	    strcpy(st_head->value.job,job);
	    printf(cursor_right5"请录入学生类别:");
	    scanf("%s",clas2);
	    strcpy(st_head->value.clas2,clas2);
*/
	    save_link(head);
	    
	    int count=0;
	    while(1){
	    system("clear");
	    display_cheek(23,86);
	    CU;
	    printf(cursor_right4"教 师 录 成 绩 与 职 务 界 面\n\n");
	    char choose=0;
	    printf(cursor_right6"是否继续录入信息(y or n)\n");
	    getchar();
	    printf(cursor_right3"您的选择是:");
	    scanf("%c",&choose);

	    if(choose=='y'){
		char ch=0;
		printf(cursor_right2"是否录入其它信息(y or n):\n");
		printf(cursor_right3"您的选择是:");
		getchar();
		scanf("%c",&ch);
		if(ch=='y'){
		    printf(cursor_right5"请录入学生班级:");
		    scanf("%s",clas1);
		    strcpy(st_head->value.clas1,clas1);
		    printf(cursor_right5"请录入学生职务:");
		    scanf("%s",job);
		    strcpy(st_head->value.job,job);
		    printf(cursor_right5"请录入学生类别:");
		    scanf("%s",clas2);
		    strcpy(st_head->value.clas2,clas2);
		    save_link(head);
		}
		else if(ch=='n')
		    return;
		else{
		    printf(cursor_right1"选择错误\n");
		    sleep(2);
		    return;
		    }
		}
	    else if(choose=='n'){
		return;
		}
	    else{
		count++;
		printf(cursor_right1"选择错误(%d)，请再次选择\n",count);
		sleep(1);
		if(count==2)
		    return ;
//		continue;
		}
	    }
	}
	else{
	    count++;
	    printf(cursor_right1"输入学号错误(%d)，请重新输入学生学号\n",
		    count);
	    sleep(2);
	    if(count==3)
		return;
	    continue;
		}
	    }
	}
    }
}
//所有学生成绩排名函数
void gpa_rank(){
    system("clear");
    display_cheek(23,86);
    CU;
    printf(cursor_right2"所 有 学 生 成 绩 排 名\n");
    ST *head=read_link();
    if(head==NULL){
	printf(cursor_right1"数据不存在\n\n");
	sleep(2);
    }
    else{
	head=select_sort_link(head);
	save2_link(head);//保存学生排名到文件st_rank.txt
	putchar('\012');
	printf(cursor_right13"帐号\t学号\t姓名\t年龄\t性别\t成绩\t班别\t职务\t类别\n\n");
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
    display_cheek(23,86);
    CU;
    printf(cursor_right2"教 师 登 录 界 面\n\n");
    printf(cursor_right3"请输入教师帐号:");
    scanf("%s",te_id);
    te=search1_link(head,te_id);
    te_head=te;

    if(te!=NULL){
	printf(cursor_right1"帐号存在\n\n");
	sleep(1);
	while(1){
	    system("clear");
	    display_cheek(23,86);
	    CU;
	    printf(cursor_right4"教 师 登 录 界 面\n\n");
	    printf(cursor_right5"请输入教师密码:");
	    scanf("%s",te_key);
	    if(!strcmp(te_key,te->value1.key)){
		printf(cursor_right6"教师登录成功\n\n");
		sleep(1);
		flag=0;
		break;
		}
	    else{
		printf(cursor_right);
		printf(cursor_right1"帐号与密码不匹配，请再次输入密码\n\n");
		key_count++;
		printf(cursor_right1"密码错误(%d)\n\n",key_count);
		sleep(2);
		if(key_count==3)
		    break;
		}
	    }
	}
	else{
	    id_count++;
	    printf(cursor_right1"帐号不存在，帐号错误(%d)\n\n",id_count);
	    sleep(2);
	    if(id_count==3)
		return ;
	    continue;
	}
    }
    
    while(1){
    system("clear");
    display_cheek(23,86);
    CU;
    printf(cursor_right6"教 师 个 人 信 息\n\n");
    printf(cursor_right5"1.查看自己信息\n\n");
    printf(cursor_right4"2.修改密码\n\n");
    printf(cursor_right3"3.查看所有学生\n\n");
    printf(cursor_right2"4.录入学生成绩/其它\n\n");
    printf(cursor_right1"5.学生成绩排名\n\n");
    printf(cursor_right7"6.退  出\n\n");
    printf(cursor_right3"请输入您的选择:");

    int choose=0;
    scanf("%d",&choose);
    switch(choose){
	case 1:te_news(te_head);break;
	case 2:te_key_change(head,te_head);break;
	case 3:display_all_student();break;
	case 4:record_mark();break;
	case 5:gpa_rank();break;
	case 6:return;
	default :printf(cursor_right1"选择错误\n\n");sleep(2);getchar();return;
	}
    }
}
//显示教师界面函数
void teacher(){
    while(1){
    system("clear");
    display_cheek(23,86);
    CU;
    printf(cursor_right6"教 师 界 面\n\n");
    printf(cursor_right5"1.注  册\n\n");
    printf(cursor_right4"2.登  录\n\n");
    printf(cursor_right3"3.退  出\n\n");
    printf(cursor_right2"请输入您的选择:");

    int choose=0;
    scanf("%d",&choose);
    switch(choose){
	case 1:te_register();break;
	case 2:te_login();break;
	case 3:return;
	default :printf(cursor_right1"选择错误\n\n");sleep(2);getchar();return;
	}
    }
}




