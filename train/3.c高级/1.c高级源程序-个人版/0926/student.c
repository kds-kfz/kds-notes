#include"student.h"
#include"link.h"

//学生注册函数
void st_link(){
    ST *head=NULL;
    int num=0;
//    int flag=0;
	int fg=0;
    head=read_link();
	if(head==NULL){
	    printf(cursor_right);
	    printf("文件无数据,需注册\n");
	    num=1000;
	    sleep(2);
	}
//	else 
//	    flag=1;
	Data d1={0};
	while(1){   
	    if(head){
		ST *tail=head;
		while(tail->next!=NULL)
		    tail=tail->next;
		num=tail->value.num;
		}
	system("clear");
	printf(cursor_right);
	printf("学生正在注册...\n");
	printf(cursor_right);
	printf("请输入个人信息\n");
	printf(cursor_right);
	printf("帐号\t密码\t姓名\t年龄\t性别\n");
	d1.num=++num;
	d1.score=0;
	printf(cursor_right);
	scanf("%s %s %s %d %s",
		d1.number,d1.key,d1.name,&d1.age,d1.sex);
	getchar();
/*	if(flag){
	    while(head->pre!=NULL)
		head=head->pre;
	    }
	    */
	head=create_link(head,d1);
	save_link(head);
	
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
    

//学生注册界面函数
void st_register(){
    while(1){
    system("clear");
    printf(cursor_right);
    printf(color_font_high"学生注册界面\n"color_end);
    printf(cursor_right);
    printf("1.学生注册\n");
    printf(cursor_right);
    printf("2.退出\n");
    printf(cursor_right);
    printf("请输入您的选择:");

    int choose=0;
    scanf("%d",&choose);
    switch(choose){
	case 1:st_link();break;
	case 2:return;
	default :printf("选择错误\n");sleep(1);break;
	}
    }
}
//查看学生个人信息函数
void st_news(ST *st_head){
    printf(cursor_right);
    printf("帐号\t密码\t学号\t姓名\t年龄\t性别\t成绩\n");
    printf(cursor_right);
    printf("%s\t%s\t%d\t%s\t%d\t%s\t%.2f\n",
	st_head->value.number,st_head->value.key,st_head->value.num,
	st_head->value.name,st_head->value.age,st_head->value.sex,
	st_head->value.score);
    sleep(5);
}
//学生修改密码函数
void st_key_change(ST *head,ST *st_head){
    int count=0;
    while(1){
    system("clear");
    char now_key[20]="0";
    printf(cursor_right);
    printf(color_font_high"学生修改密码界面\n"color_end);
    printf(cursor_right);
    printf("请输入旧密码:");
    scanf("%s",now_key);
    if(!strcmp(now_key,st_head->value.key)){
	int i=0;
	for(;i<20;i++)
	    now_key[i];
	printf(cursor_right);
	printf("请输入新密码:");
	scanf("%s",now_key);
	strcpy(st_head->value.key,now_key);
	save_link(head);
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
//学生登录函数
void st_login(){
    ST *head=NULL;
    ST *st=NULL;
    ST *st_head=NULL;

    head=read_link();
    char st_id[20]="0";
    char st_key[20]="0";
    int flag=1;
    int id_count=0;

    while(flag){
    int key_count=0;
    system("clear");
    printf(cursor_right);
    printf(color_font_high"学生登录界面\n"color_end);
    printf(cursor_right);
    printf("请输入学生帐号:");
    scanf("%s",st_id);
    st=search_link(head,st_id);
    st_head=st;

    if(st!=NULL){
	printf(cursor_right);
	printf("学号存在\n");
	sleep(1);
	while(1){
	    system("clear");
	    printf(cursor_right);
	    printf(color_font_high"学生登录界面\n"color_end);
	    printf(cursor_right);
	    printf("请输入学生密码:");
	    scanf("%s",st_key);
	    if(!strcmp(st_key,st->value.key)){
		printf(cursor_right);
		printf("学生登录成功\n");
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
    printf(color_font_high"学生个人信息\n"color_end);
    printf(cursor_right);
    printf("1.查看自己信息\n");
    printf(cursor_right);
    printf("2.修改密码\n");
    printf(cursor_right);
    printf("3.退出\n");
    printf(cursor_right);
    printf("请输入您的选择:");

    int choose=0;
    scanf("%d",&choose);
    switch(choose){
	case 1:st_news(st_head);break;
	case 2:st_key_change(head,st_head);break;
	case 3:return;
	default :printf(cursor_right);printf("选择错误\n");break;
	}
    }
}
//学生界面函数
void student(){
    while(1){
    system("clear");
    printf(cursor_right);
    printf(color_font_high"学生界面\n"color_end);
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
	case 1:st_register();break;
	case 2:st_login();break;
	case 3:return;
	default :printf(cursor_right);printf("选择错误\n");sleep(1);break;
	}
    }
}

