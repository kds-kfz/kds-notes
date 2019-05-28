#include<stdio.h>
#include<stdlib.h>
#include<mysql/mysql.h>

#include"Mysql.h"
#include"function.h"
#include<string.h>

//数据库初始化
void Mysql_init(Mysql_t *m){
    m->connect=mysql_init(&(m->mysql));
    if(m->connect==NULL){
	int ret=mysql_errno(&(m->mysql));
	printf("mysql_init error : %d\n",ret);
//	m->connect=NULL;
    }
}

//数据库连接
void Mysql_connect(Mysql_t *m,const char *s){
    m->connect=mysql_real_connect(m->connect,"localhost","root","123",s,0,NULL,0);
    if(m->connect==NULL){
	int ret=mysql_errno(&(m->mysql));
	printf("mysql_real_connect error : %d\n",ret);
//	m->connect= NULL;
    }
}

//发送命令
void Mysql_query(Mysql_t *m,const char *s){
    int ret=mysql_query(m->connect,s);
    if(ret!=0){
    printf("mysql_query error : %d\n",ret);
//    m->connect=NULL;
    }
}

//打印数据
void Mysql_show_data(Mysql_t *m,const char *s){
    Mysql_query(m,"set names utf8");//设置支持插入中文
    Mysql_query(m,s);

    m->result=mysql_store_result(m->connect);
    m->col=mysql_num_fields(m->result);

    m->fields=mysql_fetch_fields(m->result);

    for(int i=0;i<m->col;i++)
	printf("%s\t",m->fields[i].name);
    putchar(10);

    while(m->row=mysql_fetch_row(m->result)){
	for(int i=0;i<m->col;i++)
	    printf("%s\t",m->row[i]);
	putchar(10);
    }
}
/*
FILM *create_film_link(FILM *f_head,Film d1){//尾插入
    FILM *p=(FILM *)malloc(sizeof(FILM));   
    p->film_data=d1;
    p->next=NULL;
    if(f_head==NULL)
	return p;
    FILM *tail=f_head;
    while(tail->next!=NULL)//找最后一个数据地址
	tail=tail->next;
    tail->next=p;//数据地址链接
    return f_head;
}

//读取film表格,返回首地址FILM *
FILM *read_film_table(FILM *f_head,Mysql_t *m){
    Mysql_query(m,"set names utf8");
    Mysql_query(m,"select *from film");

    m->result=mysql_store_result(m->connect);
    m->col=mysql_num_fields(m->result);
    
    Film d;

    while(m->row=mysql_fetch_row(m->result)){
	memset(&d,0,sizeof(Film));
	strcpy(d.film.cinema_number,m->row[0]);
	strcpy(d.film.cinema_name,m->row[1]);
	strcpy(d.film_num,m->row[2]);
	f_head=create_film_link(f_head,d);
    }
    return f_head;
}
*/

void read_film_table(Mysql_t *m,char buf[]){
    Mysql_query(m,"set names utf8");
    Mysql_query(m,"select *from film");
    m->result=mysql_store_result(m->connect);
    m->col=mysql_num_fields(m->result);
    int max=0;
    while(m->row=mysql_fetch_row(m->result)){
	for(int i=0;i<m->col;i++){

	    strcpy(buf+max,m->row[i]);
	    max+=sizeof(m->row[i]);
	    strcpy(buf,",");
	    max+=1;
	}
	strcpy(buf+max,"#");
	max+=1;
    }
}


//释放数据库
void My_free(Mysql_t *m){
    mysql_free_result(m->result);
    mysql_close(m->connect);
}

void Mysql_insert_data(Mysql_t *m,const char *s1,const char *s2){
    Mysql_query(m,s1);
}
//删除某个表格满足条件的内容
void delete_table(Mysql_t *m,const char *s1,const char *s2){
    char sql_buf[1024];
    sprintf(sql_buf,"delete from %s where %s;",s1,s2);
    Mysql_query(m,sql_buf);
}

//清空某个表格内容
void clear_table(Mysql_t *m,const char *s){
    Mysql_query(m,s);
}
//删除所有表格
void delete_all_table(Mysql_t *m){
    char *sql1="drop table person";
    char *sql2="drop table film";
    char *sql3="drop table movie";
//    char *sql4="drop table filmyard";
    char *sql5="drop table filmseat";
    char *sql6="drop table record";
    Mysql_query(m,sql1);
    Mysql_query(m,sql2);
    Mysql_query(m,sql3);
//    Mysql_query(m,sql4);
    Mysql_query(m,sql5);
    Mysql_query(m,sql6);
}

//创建所有表格
void create_all_table(Mysql_t *m){
    //创建所有表格前删除已有的表格
    delete_all_table(m);

    //创建person表格,存放顾客和管理员信息
    char *sql1="create table person(number varchar(15),account varchar(20),password varchar(15),age varchar(15),sex varchar(15),telephone varchar(20),address varchar(20),wallet varchar(15),status varchar(15))";
    
    //创建film表格，存放电影院编号+院名+电影总数
    char *sql2="create table film(number int,name varchar(20),count int)";
    
    //创建movie表格,存放电影信息
    char *sql3="create table movie(m_number int,m_name varchar(20),c_number int,c_name varchar(20),p_time varchar(15),s_time varchar(15),m_price int,m_count int,m_yard int)";
    
    //创建filmyard表格,存放所有电影院场地
    char *sql4="create table filmyard(number int,yard varchar(10))";

    //创建filmseat表格,存放所有电影院座位
    char *sql5="create table filmseat(c_number int,c_yard int,seat varchar(50))";

    //创建购票记录record表格
    char *sql6="create table record(p_number int,p_name varchar(20),m_number int,m_name varchar(20),c_number int,c_name varchar(20),p_time varchar(15),s_time varchar(15),m_price int,seat int,m_yard int,b_da DATE,b_ti TIME)";

    Mysql_query(m,sql1);//创建person表格
    Mysql_query(m,sql2);//创建film表格
    Mysql_query(m,sql3);//创建movie表格
//    Mysql_query(m,sql4);//创建filmyard表格
    Mysql_query(m,sql5);//创建filmseat表格
    Mysql_query(m,sql6);//创建record表格
}

//向person表格增加内容
void insert_person(Mysql_t *m,Custom *p){
    //增加前先删掉该条信息
    char delete_buf[1024];
    sprintf(delete_buf,"delete from person where number='%s';",p->number);
    Mysql_query(m,delete_buf);

    char sql_buf[1024];
    sprintf(sql_buf,"insert into person(number,account,password,age,sex,telephone,address,wallet,status) values('%s','%s','%s','%s','%s','%s','%s','%s','%s');",
    p->number,p->account,p->password,p->age,p->sex,
    p->telephone,p->address,p->wallet,p->status);
    Mysql_query(m,"set names utf8");//设置支持插入中文
    Mysql_query(m,sql_buf);
}

//向film表格增加内容
void insert_film(Mysql_t *m,Film *p){
    //增加前先删掉该条信息
    char delete_buf[1024];
    sprintf(delete_buf,"delete from film where number=%d;",p->film.cinema_number);
    Mysql_query(m,delete_buf);

    char sql_buf[1024];
    sprintf(sql_buf,"insert into film(number,name,count) values(%d,'%s',%d);",
    p->film.cinema_number,p->film.cinema_name,p->film_num);
    Mysql_query(m,"set names utf8");//设置支持插入中文
    Mysql_query(m,sql_buf);
}

//向movie表格增加内容
void insert_movie(Mysql_t *m,Movie *p){
    //增加前先删掉该条信息
    char delete_buf[1024];
    sprintf(delete_buf,"delete from movie where m_number=%d;",p->movie_number);
    Mysql_query(m,delete_buf);

    char sql_buf[1024];
    sprintf(sql_buf,"insert into movie(m_number,m_name,c_number,c_name,p_time,s_time,m_price,m_count,m_yard) values(%d,'%s',%d,'%s','%s','%s',%d,%d,%d);",
    p->movie_number,p->movie_name,p->c_number,
    p->c_name,p->play_time,p->show_time,
    p->movie_price,p->ticket_count,p->play_yard);
    Mysql_query(m,"set names utf8");//设置支持插入中文
    Mysql_query(m,sql_buf);
}

//向filmyard表格增加内容
void insert_filmyard(Mysql_t *m,Film_t *p){
    //增加前先删掉该条信息
    char delete_buf[1024];
    sprintf(delete_buf,"delete from filmyard where number=%d;",p->c_number);
    Mysql_query(m,delete_buf);

    char buf[11];
    memset(buf,'0',10);
    int i=0;
    for(;i<10;i++)
	if(p->film_yard[i]==1)
	    buf[i]='1';
	else 
	    buf[i]='0';
    buf[i]='\0';
    char sql_buf[1024];
    sprintf(sql_buf,"insert into filmyard(number,yard) values(%d,'%s')",
    p->c_number,buf);
    Mysql_query(m,"set names utf8");//设置支持插入中文
    Mysql_query(m,sql_buf);
}

//向filmseat表格增加内容
void insert_filmseat(Mysql_t *m,Film_t *p){
    //增加前先删掉该条信息
    char delete_buf[1024];
    sprintf(delete_buf,"delete from filmseat where c_number=%d;",p->c_number);
    Mysql_query(m,delete_buf);

    char buf[51];
    char yard[11];
    memset(buf,'0',50);
    int i=0;
    for(;i<50;i++)
	if(p->film_seat[i]==1)
	    buf[i]='1';
	else 
	    buf[i]='0';
    buf[i]='\0';
/*    
    for(;i<10;i++)
	p->film_yard[i]=49+i;
    film_yard[i]='\0';
*/    
    for(int i=0;i<10;i++){
    char sql_buf[1024];
    sprintf(sql_buf,"insert into filmseat(c_number,c_yard,seat) values(%d,%d,'%s');",
    p->c_number,p->film_yard[i],buf);
    Mysql_query(m,"set names utf8");//设置支持插入中文
    Mysql_query(m,sql_buf);
    }
}

//向record表格插入电影票
void insert_record_data(Mysql_t *m,Ticket *t){
    char sql_buf[1024];
    sprintf(sql_buf,"insert into record(p_number,p_name,m_number,m_name,c_number,c_name,p_time,s_time,m_price,seat,m_yard,b_da,b_ti) values(%d,'%s',%d,'%s',%d,'%s','%s','%s',%d,%d,%d,'%s','%s');",
    t->number,t->name,t->ticket.movie_number,t->ticket.movie_name,
    t->ticket.c_number,t->ticket.c_name,t->ticket.play_time,
    t->ticket.show_time,t->ticket.movie_price,t->ticket.play_yard,t->t_seat,
    t->d_date,t->t_time);
    Mysql_query(m,"set names utf8");//设置支持插入中文
    Mysql_query(m,sql_buf);
}

void updata_film_seat(Mysql_t *m,int c_num,int y_num,int s_num){
    //先查找
    char seek_buf[512];
    char film_seat[51];
    sprintf(seek_buf,"select *from filmseat where c_number=%d and c_yard=%d",c_num,y_num);    
    Mysql_query(m,seek_buf);
    m->result=mysql_store_result(m->connect);
    m->row=mysql_fetch_row(m->result);
    strcpy(film_seat,m->row[2]);
//    printf("%s\n",film_seat);

    //增加前先删掉该条信息
    char delete_buf[1024];
    sprintf(delete_buf,"delete from filmseat where c_number=%d and c_yard=%d;",c_num,y_num);
    Mysql_query(m,delete_buf);
    
//    memcpy(&film_seat[s_num-1],"1",1);
    film_seat[s_num-1]='1';
//    film_seat[50]='\0';
//    printf("%s\n",film_seat);

    char sql_buf[1024];
    sprintf(sql_buf,"insert into filmseat(c_number,c_yard,seat) values(%d,%d,'%s');",
    c_num,y_num,film_seat);
    Mysql_query(m,"set names utf8");//设置支持插入中文
    Mysql_query(m,sql_buf);

}

void updata_movie_data(Mysql_t *m,int m_num,int n){
    char seek_buf[512];
    sprintf(seek_buf,"select *from movie where m_number=%d",m_num);
//    printf("%d\n",m_num);
    Mysql_query(m,seek_buf);
    m->result=mysql_store_result(m->connect);
    m->row=mysql_fetch_row(m->result);
    int count=atoi(m->row[7]);
    count-=n;
//    printf("%d\n",count);

    char sql[512];
    sprintf(sql,"update movie set m_count=%d where m_number=%d;",count,m_num);    
    Mysql_query(m,seek_buf);
    
}

void updata_custom_data(Mysql_t *m,int m_num,int n){
    char seek_buf[512];
    char str1[15];
    int_to_ch(m_num,str1,15);//顾客编号
    sprintf(seek_buf,"select *from person where number='%s';",str1);
    char *p="select *from person where number='1001';";
    printf("%s\n",str1);
    Mysql_query(m,p);
    m->result=mysql_store_result(m->connect);
    m->row=mysql_fetch_row(m->result);
    printf("%s\n",m->row[6]);
    int count=atoi(m->row[7]);
    printf("------%d\n",count);
    count-=n;
    printf("%d\n",count);
    char str2[15];
    int_to_ch(count,str2,15);//浅
    printf("%s\n",str2);

    char sql[512];
    sprintf(sql,"update person set wallet='%s' where number='%s';",str2,str1);    
    Mysql_query(m,seek_buf);
    
}
//检查数person表格number与帐号是否存在
char check_person(Mysql_t *m,Custom *p){
    char check_buf[1024];
    sprintf(check_buf,"select count(*) from person where number='%s' and account='%s';",p->number,p->account);
    Mysql_query(m,check_buf);
    
    m->result=mysql_store_result(m->connect);
    MYSQL_ROW ret=mysql_fetch_row(m->result);//获取满足条件的行数
    return *ret[0];
}

void insert_all_data(Mysql_t *m){
    Custom pe1={"1001","lisi","key123","24","男","12345678901","深大","100","client"};
    Custom pe2={"1002","keke","key123","24","男","12345678901","深大","100","client"};
    
    Film pe3={3001,"大地影院",50};
    Film pe4={3002,"乐巢影院",50};

    Movie pe5={4001,"变形金刚1",3001,"大地影院","120分钟","2017-12-18",89,40,1};
    Movie pe6={4002,"变形金刚2",3001,"大地影院","120分钟","2017-12-18",89,40,1};
    Movie pe7={4003,"变形金刚3",3002,"乐巢影院","120分钟","2017-12-18",89,40,1};
    Movie pe8={4004,"变形金刚4",3002,"乐巢影院","120分钟","2017-12-18",89,40,1};

    Film_t pe9={3001,{1,2,3,4,5,6,7,8,9,10},{0}};
    Film_t pe10={3002,{1,2,3,4,5,6,7,8,9,10},{0}};

    Custom *p1=&pe1;
    insert_person(m,p1);
    p1=&pe2;
    insert_person(m,p1);
    
    Film *p2=&pe3;
    insert_film(m,p2);
    p2=&pe4;
    insert_film(m,p2);

    Movie *p3=&pe5;
    insert_movie(m,p3);
    p3=&pe6;
    insert_movie(m,p3);
    p3=&pe7;
    insert_movie(m,p3);
    p3=&pe8;
    insert_movie(m,p3);
    
    Film_t *p4=&pe9;
    insert_filmseat(m,p4);
    p4=&pe10;
    insert_filmseat(m,p4);
}

//查找数据库，编号+帐号+密码 与 登录者是否完全相同
char *test_new_custom(Mysql_t *m,Custom *p){
    Mysql_query(m,"set names utf8");
    char test_buf[1024];
    //sprintf(test_buf,"select *from person where account='%s' and password='%s' and number='%s';",
    sprintf(test_buf,"select *from person;");

    Mysql_query(m,test_buf);

    m->result=mysql_store_result(m->connect);
    m->col=mysql_num_fields(m->result);
    
    int a,b;//帐号，密码标志位
    while(m->row=mysql_fetch_row(m->result)){
	a=0;b=0;
	for(int i=0;i<m->col;i++){
	    if(!strcmp(p->account,m->row[i])&&i==1)
		a=1;
	    else if(!strcmp(p->password,m->row[i])&&i==2)
		b=1;
	}
	if(a==1&&b==1)
	    break;
    }
    if(a==1&&b==1)
	return "#1";
    else
	return "#2";
}

void seek_custom_news(Mysql_t *m,Custom *p1,Custom *p2){
    Mysql_query(m,"set names utf8");
    char test_buf[1024];
    sprintf(test_buf,"select *from person where account='%s' and password='%s';",
    p1->account,p1->password);
    Mysql_query(m,test_buf);

    m->result=mysql_store_result(m->connect);
    m->row=mysql_fetch_row(m->result);
    strcpy(p2->number,m->row[0]);
    strcpy(p2->account,m->row[1]);
    strcpy(p2->password,m->row[2]);
    strcpy(p2->age,m->row[3]);
    strcpy(p2->sex,m->row[4]);
    strcpy(p2->telephone,m->row[5]);
    strcpy(p2->address,m->row[6]);
    strcpy(p2->wallet,m->row[7]);
    strcpy(p2->status,m->row[8]);
}
