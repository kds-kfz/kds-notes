#include<stdio.h>
#include<stdlib.h>
#include<string.h>
//1.c
//哈希表与链地址法
//定义数据类型
typedef struct d{
    char name[20];
    struct d *next;
}ST;
//定义结构体数组
ST *arr[5]={NULL};
//构造哈希函数
int haxi(char ch){
    switch(ch){
	case 'a':return 0;break;
	case 'b':return 1;break;
	case 'c':return 2;break;
	case 'd':return 3;break;
	default :return 4;break;
    }
}
//创链表
void creat_haxi_link(const char *p){
    int i=haxi(*p);
    ST *s=(ST *)malloc(sizeof(ST));
    strcpy(s->name,p);
    s->next=NULL;
    if(arr[i]==NULL)
	arr[i]=s;
    else{
	ST *t=arr[i];
	while(t->next!=NULL)
	    t=t->next;
	t->next=s;
    }
}
//打印对应字母所有数据
void print(char ch){
    int i=haxi(ch);
    ST *head=arr[i];
    while(head){
	printf("%s ",head->name);
	head=head->next;
    }
    printf("\n");
}
//打印前num个数据
void print_all(int num){
    int i=0;
    ST *head=NULL;
    printf("打印所有数据:\n");
    for(;i<num;i++)
    {
	int count=0;
	head=arr[i];
	while(head){
	    count++;
	    printf("%s ",head->name);
	    head=head->next;
	}
	if(arr[i]){
	    printf("人数：%d",count);
	    putchar(10);
	}
    }
}
//释放前num个数据
void free_link(int num){
    int i=0;
    ST *head=NULL;
    printf("释放前%d个数据:\n",num);
    for(;i<num;i++)
    {
	head=arr[i];
	ST *t=NULL;
	while(head){
	    t=head;
	    head=head->next;
	    free(t);
	}
	arr[i]=NULL;
    }
    printf("free success\n");
}
//释放第num个数据
void free1_link(int num){
    ST *head=NULL;
    printf("释放第%d个数据:\n",num+1);
    head=arr[num];
    ST *t=NULL;
//    ST *t1=head;
    while(head){
	t=head;
	head=head->next;
	free(t);
    }
    arr[num]=NULL;
    printf("free success\n");
}
//统计所有数据，首字母对应数据个数
void haxi_count(){
    int i=0;
    for(;i<5;i++){
	ST *head=arr[i];
	if(i==4)
	    printf("其它人数是:");
	else
	    printf("%c的人数是:",head->name[0]);
	int count=0;
	while(head){
	    count++;
	    head=head->next;
	}
	printf(" %d\n",count);
    }
}
//释放全部数据
void free_all(){
    int i=0;
    for(;i<5;i++){
	ST *head=arr[i];
	ST *t=NULL;
	while(head){
	    t=head;
	    head=head->next;
	    free(t);
	}
	arr[i]=NULL;
    }
    printf("free all success\n");
}
//打印所有首字母里所有数据
void printf_all(){
    int i=0;
    for(;i<5;i++){
	ST *head=arr[i];
	if(i==4)
	    printf("其它有:");
	else
	    printf("%c 里有:",head->name[0]);
	while(head){
	    printf("%s ",head->name);
	    head=head->next;
	}
	putchar(012);
    }
}

int main(){
    //创哈希表数据
    creat_haxi_link("ab1");
    creat_haxi_link("ab2");
    creat_haxi_link("bb1");
    creat_haxi_link("bb2");
    creat_haxi_link("cb1");
    creat_haxi_link("cb2");
    creat_haxi_link("dr1");
    creat_haxi_link("eb1");
    creat_haxi_link("fb1");
    creat_haxi_link("gb1");

    //打印所有首字母里所有数据
    printf_all();
    //统计所有数据，首字母对应数据个数
    haxi_count();

    //打印前num个数据
    print_all(5);

    //释放前num个数据
    free_link(2);

    //释放第num个数据
    free1_link(4);

    //释放全部数据
    free_all();

/*
    while(1){
    char ch=0;
    printf("输入1个字母:\n");
    scanf("%c",&ch);
    getchar();
    //打印对应字母所有数据
    print(ch);
    }
    */
    return 0;
}
