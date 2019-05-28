#include"manage.h"
#include"student.h"
#include"teacher.h"
#include"font.h"
//manage.c

//学生管理系统主界面
void menu(){
    cursor_move(x_seat,y_seat);
    printf(cursor_right1"学生管理系统(已升级)\n\n");
    printf(cursor_right2"系统版本:文件版v10.3\n\n");
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
    cursor_move(x_seat,y_seat);
    printf(cursor_right1"管 理 员 修 改 信 息 界 面\n\n");
    FILE *fp=fopen(".admin.txt","r+");
    char id1[20]="0";
    char key1[20]="0";
    char key2[20]="0";
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
	cursor_move(x_seat,y_seat);
	printf(cursor_right1"管 理 员 修 改 信 息 界 面\n\n");

	FILE *fp1=fopen(".admin.txt","w");

	printf(cursor_right2"请再次输入密码:");
	getchar();
	scanf("%s",key1);

	int k=0;//对验证密码做简单加密
	while(key1[k]){
	    if(key1[k]>='0'&&key1[k]<='9')
		key1[k]^='a';
	    if(key1[k]>='a'&&key1[k]<='z')
		key1[k]^='9';
	    if(key1[k]>='A'&&key1[k]<='Z')
		key1[k]^='1';
	    k++;
	    }
	
	if(!strcmp(key,key1)){//校验旧密码成功
	    putchar(10);
	    getchar();
	    printf(cursor_right3"请输入新帐号:");
	    scanf("%s",id1);
	    putchar(10);
	    getchar();
	    printf(cursor_right6"请输入新密码:");
	    scanf("%s",key1);
	    
	    
	    int k=0;//对管理员密码做简单加密
	    while(key1[k]){
		if(key1[k]>='0'&&key1[k]<='9')
		    key1[k]^='a';
		if(key1[k]>='a'&&key1[k]<='z')
		    key1[k]^='9';
		if(key1[k]>='A'&&key1[k]<='Z')
		    key1[k]^='1';
		k++;
		}
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
	cursor_move(x_seat,y_seat);
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
	printf(cursor_right13"帐号\t工号\t姓名\t年龄\t性别\t职务\t班级\t科目\n\n");
	printf1_link_pre(head);
	sleep(5);
    }
}

//注销学生函数
void del_student(){
    while(1){
    cursor_move(x_seat,y_seat);
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
    cursor_move(x_seat,y_seat);
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

//管理员给教师录入职位函数
void record_te_job(){
//    while(1){
//    cursor_move(x_seat,y_seat);
    int count=0;
    
    while(1){
    cursor_move(x_seat,y_seat);
    int te_id=0;
    int fg=0;
    printf(cursor_right4"管 理 员 录 职 务 班 级 科 目 界 面\n\n");
    printf(cursor_right2"请输入要录入职务的教师工号:");
    scanf("%d",&te_id);
    
    ST1 *head=read1_link();
    ST1 *te_head=head;
    if(te_head==NULL){
	printf(cursor_right1"文件不存在\n\n");
	sleep(2);
    }
    else{
	te_head=search3_link(te_head,te_id);
	if(te_head){
	    printf(cursor_right3"该工号教师存在\n\n");
	    char job[20]="0";
	    char clas[10]="0";
	    char course[10]="0";
	    printf(cursor_right4"姓名:%s 职务:%s 班级:%s 科目:%s\n\n",
		    te_head->value1.name,te_head->value1.job,
		    te_head->value1.clas,te_head->value1.course);
	    printf(cursor_right5"请录入教师职务:");
	    scanf("%s",job);
	    strcpy(te_head->value1.job,job);
	    save1_link(head);
	    
	    int count=0;
	    while(1){
	    cursor_move(x_seat,y_seat);	
	    printf(cursor_right4"管 理 员 录 职 务 班 级 科 目 界 面\n\n");
	    char choose=0;
	    printf(cursor_right6"是否继续录入教师信息(y or n)\n");
	    getchar();
	    printf(cursor_right3"您的选择是:");
	    scanf("%c",&choose);

	    if(choose=='y'){
		char ch=0;
		printf(cursor_right6"是否录入教师其它信息(y or n)\n");
		printf(cursor_right5"您的选择是:");
		getchar();
		scanf("%c",&ch);
		if(ch=='y'){
		    printf(cursor_right5"请录入教师任课班级:");
		    scanf("%s",clas);
		    strcpy(te_head->value1.clas,clas);
		    printf(cursor_right5"请录入教师任课科目:");
		    scanf("%s",course);
		    strcpy(te_head->value1.course,course);
		    save1_link(head);
		}
		else if(ch=='n')
		    break;
		else{
		    printf(cursor_right1"选择错误\n");
		    sleep(2);
		    return ;
		    }
		}
	    else if(choose=='n')
		break;
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
	    printf(cursor_right1"输入工号错误(%d)，请重新输入教师工号\n",
		    count);
	    sleep(2);
	    if(count==2)
		return;
//	    continue;
		}
	    }
	}
//    }
}

//新生一键排班函数,也可用于单个转校生
void take_clas(){
    ST *head=read_link();
    ST *st=head;
    if(head){
    cursor_move(x_seat,y_seat);
    printf(cursor_right2"管 理 员 排 班 界 面\n\n");
    printf(cursor_right3"学生文件存在\n\n");
    sleep(2);
	while(1){
	char choose=0;
	int flag=1;
	cursor_move(x_seat,y_seat);
	printf(cursor_right2"管 理 员 排 班 界 面\n\n");
	printf(cursor_right3"1.所有学生一键排班(慎用)\n\n");
	printf(cursor_right4"2.转入学生排班\n\n");
	printf(cursor_right5"3.退  出\n\n");
	printf(cursor_right6"请输入您的选择:");
	scanf("%c",&choose);
	if(choose=='1'){
	    char clas[10]="0";
	    char choose=0;
	    printf(cursor_right6"请输入排班班级:");
	    scanf("%s",clas);

	    printf(cursor_right6"是否确认目前操作?(y or n):");
	    getchar();
	    scanf("%c",&choose);
	    if(choose=='y'){
	    while(st->next!=NULL){
	    strcpy(st->value.clas1,clas);
	    st=st->next;
		}
	    strcpy(st->value.clas1,clas);//给最后一个学生录班级
	    }
	    else
		break;
	}
	else if(choose=='2'){
	    while(flag){
	    cursor_move(x_seat,y_seat);
	    printf(cursor_right2"新 生 转 班 界 面\n\n");
	    int st_num=0;
	    ST *st_head=head;
	    printf(cursor_right6"请输入转入学生学号:");
	    scanf("%d",&st_num);
	    st_head=search2_link(st_head,st_num);
	    if(st_head){
		printf(cursor_right6"该学号学生存在\n");
		char clas[10]="0";
		printf(cursor_right6"请输入转入学生班级:");
		scanf("%s",clas);
		strcpy(st_head->value.clas1,clas);
		char ch=0;
		int count=0;
		printf(cursor_right6"是否继续给其它学生转班(y or n)\n");
		while(1){
		printf(cursor_right6"您的选择是:");
		getchar();
		scanf("%c",&ch);
		if(ch=='y')
		    break;
		else if(ch=='n'){
		    flag=0;
		    break;}
		else{
		    count++;
		    printf(cursor_right1"选择错误(%d)\n\n",count);
		    sleep(2);
		    if(count==2){
			flag=0;
			break;
			}
		    }
		}
	    }
	    else{
		printf(cursor_right6"该学号学不生存在，需要学生注册\n");
		sleep(2);
		char ch=0;
		int count=0;
		printf(cursor_right6"是否继续给其它学生转班(y or n)\n");
		while(1){
		printf(cursor_right6"您的选择是:");
		getchar();
		scanf("%c",&ch);
		if(ch=='y')
		    break;
		else if(ch=='n'){
		    flag=0;
		    break;}
		else{
		    count++;
		    printf(cursor_right1"选择错误(%d)\n\n",count);
		    sleep(2);
		    if(count==2){
			flag=0;
			break;
			    }
			}
		    }//转班输入学生学号错误，循环
		}//
	    }//转班循环
	}
	else if(choose=='3')
	    return;
	else{
	    printf(cursor_right1"选择错误\n");
	    sleep(2);
	    getchar();//接收‘选择错误’回车，否则下次选择会出错
	    return;	    
	    }
	save_link(head);
	}
    }//学生文件存在
    else{
	printf(cursor_right1"学生文件不存在\n\n");
	sleep(2);
	return;
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
	    cursor_move(x_seat,y_seat);
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

	    int k=0;//对管理员密码做简单解密
	    while(key1[k]){
		if(key1[k]>='0'&&key1[k]<='9')
		    key1[k]^='a';
		if(key1[k]>='a'&&key1[k]<='z')
		    key1[k]^='9';
		if(key1[k]>='A'&&key1[k]<='Z')
		    key1[k]^='1';
		k++;
	    }
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
		if(count==2){
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
	printf(cursor_right2"需要管理员注册\n\n");
	sleep(2);
        cursor_move(x_seat,y_seat);
	printf(cursor_right3"管 理 员 注 册 界 面\n\n");

	FILE *fp2=fopen(".admin.txt","w");
	char id1[20]="0";//保存新帐号  
	char key1[20]="0";//保存新密码
	printf(cursor_right4"请输入管理员帐号:");
	scanf("%s",id1);
	putchar(10);
	getchar();
	printf(cursor_right3"请输入管理员密码:");
	scanf("%s",key1);

	int k=0;//对管理员密码做简单加密
	while(key1[k]){
	    if(key1[k]>='0'&&key1[k]<='9')
		key1[k]^='a';
	    if(key1[k]>='a'&&key1[k]<='z')
		key1[k]^='9';
	    if(key1[k]>='A'&&key1[k]<='Z')
		key1[k]^='1';
	    k++;
	}
	fprintf(fp2,"%s\n",id1);//向文件写入新帐号
	fprintf(fp2,"%s",key1);//向文件写入新密码
	fclose(fp2);
	printf(cursor_right5"管理员注册成功\n\n");
	return;
    }
    //管理员登录成功后，进入管理界面
    while(1){
    cursor_move(x_seat,y_seat);
    printf(cursor_right1"管 理 员 管 理 界 面\n\n");
    printf(cursor_right2"1.修改管理员信息\n\n");
    printf(cursor_right3"2.新生一键排班\n\n");
    printf(cursor_right3"3.查看所有学生\n\n");
    printf(cursor_right4"4.查看所有教师\n\n");
    printf(cursor_right4"5.录入教师职位/其它\n\n");
    printf(cursor_right5"6.注销学生\n\n");
    printf(cursor_right6"7.注销教师\n\n");
    printf(cursor_right2"8.退  出\n\n");
    printf(cursor_right3"请输入您的选择:");

//    int choose=0;
//    scanf("%d",&choose);
    char choose=menu_choose();
    switch(choose){
	case '1':admin_change();break;
	case '2':take_clas();break;
	case '3':display_all_student();break;
	case '4':display_all_teacher();break;
	case '5':record_te_job();break;
	case '6':del_student();break;
	case '7':del_teacher();break;
	case '8':return;
	default :printf(cursor_right1"选择错误\n");sleep(2);break;
	}
    }
}
//管理员界面函数
void manager(){
    while(1){
    cursor_move(x_seat,y_seat);
    printf(cursor_right5"管 理 员 界 面\n\n"color_end);
    printf(cursor_right4"1.登  录\n\n");
    printf(cursor_right3"2.退  出\n\n");
    printf(cursor_right2"请输入您的选择:");

//    int choose=0;
//    scanf("%d",&choose);
    char choose=menu_choose();
    switch(choose){
	case '1':ma_login();break;
	case '2':return;
	default:printf(cursor_right1"选择错误\n");sleep(2);break;
	}
    }
}

//学生系统函数
void my_system(){
    while(1){
    menu();

//    int choose=0;
//    scanf("%d",&choose);
    char choose=menu_choose();
    switch(choose){
	case '1':student();break;
	case '2':teacher();break;
	case '3':manager();break;
	case '4':exit(-1);
	default :printf(cursor_right1"选择错误\n");sleep(2);break;
	}
    }
}


