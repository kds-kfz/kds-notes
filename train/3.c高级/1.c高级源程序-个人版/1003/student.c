#include"student.h"
#include"link.h"
#include"font.h"
//student.c

//输入前，把缓存区里回车前的所有内容全部接收
void clear_buf(){
    char ch='0';
    while((ch=getchar())!='\n');
}

//学生注册函数
void st_link(){
    ST *head=NULL;
    int num=0;
//    int fg=0;
    head=read_link();
	if(head==NULL){
	    printf(cursor_right2"文件无数据,需注册\n");
	    num=1000;
	    sleep(2);
	}
	Data d1={0};
	Data *p=&d1;//定义一个学生结构体指针
	while(1){   
	    int count=0;
	    if(head){//找链表最后一个数，获取学号
		ST *tail=head;
		while(tail->next!=NULL)
		    tail=tail->next;
		num=tail->value.num;
		}
	cursor_move(x_seat,y_seat);
	printf(cursor_right4"学生正在注册...\n\n");
	printf(cursor_right2"请输入个人信息\n\n");
	printf(cursor_right11"帐号\t密码\t姓名\t年龄\t性别\n\n");
	d1.num=++num;
	d1.chinese=0;//默认成绩无
	d1.math=0;//默认成绩无
	d1.english=0;//默认成绩无
	strcpy(d1.job,"无");//默认职务无
	strcpy(d1.clas1,"无");//默认班级无
	strcpy(d1.clas2,"无");//默认类别无
	printf(cursor_right11);
	scanf("%s %s %s %d %s",d1.number,d1.key,d1.name,&d1.age,d1.sex);
	getchar();
	//此时需要判断学生新注册的帐号是否重复
	ST *repeat=head;
	repeat=search_link(repeat,p->number);
	if(repeat){
	    printf(cursor_right1"学生帐号出现重复，注册失败\n\n");
	    sleep(2);
	}
	else
	    head=create_link(head,d1);
	save_link(head);
	printf(cursor_right5"是否继续注册(y or n)\n\n");
	while(1){
	char choose=0;
	printf(cursor_right3"您的选择是:");
	scanf("%c",&choose);
	getchar();
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
    

//学生注册界面函数
void st_register(){
    while(1){
    cursor_move(x_seat,y_seat);
    printf(cursor_right5"学 生 注 册 界 面\n\n");
    printf(cursor_right2"1.学生注册\n\n");
    printf(cursor_right6"2.退  出\n\n");
    printf(cursor_right4"请输入您的选择:");

//    int choose=0;
//    scanf("%d",&choose);
    char choose=menu_choose();
    switch(choose){
	case '1':st_link();break;
	case '2':return;
	default :printf(cursor_right1"选择错误\n");sleep(2);break;
	}
    }
}
//查看学生个人信息函数
void st_news(ST *st_head){
    cursor_move(x_seat,y_seat);
//    putchar(10);
    printf(cursor_right3"学 生 个 人 信 息\n\n");

    printf(cursor_right1"~帐 号~：%s\n",st_head->value.number);
    printf(cursor_right2"~密 码~：%s\n",st_head->value.key);
    printf(cursor_right3"~学 号~：%d\n",st_head->value.num);
    printf(cursor_right4"~姓 名~：%s\n",st_head->value.name);
    printf(cursor_right5"~年 龄~：%d\n",st_head->value.age);
    printf(cursor_right6"~性 别~：%s\n",st_head->value.sex);
    printf(cursor_right7"~语 文~：%.2f\n",st_head->value.chinese);
    printf(cursor_right8"~数 学~：%.2f\n",st_head->value.math);
    printf(cursor_right9"~英 语~：%.2f\n",st_head->value.english);
    printf(cursor_right5"~班 别~：%s\n",st_head->value.clas1);
    printf(cursor_right6"~职 务~：%s\n",st_head->value.job);
    printf(cursor_right7"~类 别~：%s\n",st_head->value.clas2);
    sleep(5);
}
//学生修改密码函数
void st_key_change(ST *head,ST *st_head){
    int count=0;
    while(1){
    cursor_move(x_seat,y_seat);
    char now_key[20]="0";
    printf(cursor_right4"学 生 修 改 密 码 界 面\n\n");
    printf(cursor_right6"请输入旧密码:");
    scanf("%s",now_key);
    if(!strcmp(now_key,st_head->value.key)){
	int i=0;
	for(;i<20;i++)
	    now_key[i];
	printf(cursor_right3"请输入新密码:");
	scanf("%s",now_key);
	strcpy(st_head->value.key,now_key);
	save_link(head);
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
    cursor_move(x_seat,y_seat);
    printf(cursor_right5"学 生 登 录 界 面\n\n");
    printf(cursor_right3"请输入学生帐号:");
    scanf("%s",st_id);
    st=search_link(head,st_id);
    st_head=st;

    if(st!=NULL){
	printf(cursor_right6"学号存在\n\n");
	sleep(1);
	while(1){
	    cursor_move(x_seat,y_seat);
	    printf(cursor_right4"学 生 登 录 界 面\n\n");
	    printf(cursor_right2"请输入学生密码:");
	    scanf("%s",st_key);
	    if(!strcmp(st_key,st->value.key)){
		printf(cursor_right3"学生登录成功\n");
		sleep(1);
		flag=0;
		break;
		}
	    else{
		printf(cursor_right1"帐号与密码不匹配，请再次输入密码\n");
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
	    if(id_count==2)
		return ;
//	    continue;
	}
    }
    
    clear_buf();//输入前清空回车前所有内容
    while(1){
    cursor_move(x_seat,y_seat);
    printf(cursor_right2"学 生 个 人 信 息\n\n");
    printf(cursor_right3"1.查看自己信息\n\n");
    printf(cursor_right4"2.修改密码\n\n");
    printf(cursor_right5"3.退  出\n\n");
    printf(cursor_right6"请输入您的选择:");

//    int choose=0;
//    scanf("%d",&choose);
    char choose=menu_choose();//获取键盘输入
    switch(choose){
	case '1':st_news(st_head);break;
	case '2':st_key_change(head,st_head);break;
	case '3':return;
	default :printf(cursor_right1"选择错误\n");sleep(2);break;
	}
    }
}
//学生界面函数
void student(){
    while(1){
    cursor_move(x_seat,y_seat);
    printf(cursor_right6"学 生 界 面\n\n");
    printf(cursor_right5"1.注  册\n\n");
    printf(cursor_right4"2.登  录\n\n");
    printf(cursor_right3"3.退  出\n\n");
    printf(cursor_right2"请输入您的选择:");

//    int choose=0;
//    scanf("%d",&choose);
    char choose=menu_choose();
    switch(choose){
	case '1':st_register();break;
	case '2':st_login();break;
	case '3':return;
	default :printf(cursor_right1"选择错误\n");sleep(2);break;
	}
    }
}

