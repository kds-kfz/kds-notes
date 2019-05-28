#include"stdio.h"
#include<string.h>
//2.c
//形参为结构体类型的函数，传递指针
struct st{
    char name[20];
    int age;
};

struct te{
    int num;
    struct st value;
};

void get_st(struct st *pa,int n){
    int i=0;
    printf("student information input----------\n\n");
    /*
// 方式1--------------------------------------------
    for(;i<n;i++){
	scanf("%s %d",pa[i].name,&pa[i].age);
//	scanf("%s %d",(pa+i).name,(pa+i).age);
	}
    */
// 方式2--------------------------------------------
    for(i=0;i<n;i++,pa++){
	scanf("%s %d",pa->name,pa->age);
//	scanf("%s %d",(*pa).name,&(*pa).age);
    }
}

void get_te(struct te *pb,struct st *pa,int n){
    int tn=1700;
    int i=0;
    printf("teacher get information ----------\n\n");
// 方式1--------------------------------------------
    /*
    for(;i<n;i++){
	(*(pb+i)).num = ++tn;
	strcpy((*(pb+i)).value.name,(*(pa+i)).name);
	(*(pb+i)).value.age=(*(pa+i)).age;
    }
    */
// 方式2--------------------------------------------
    /*
    for(;i<n;i++){
	(pb+i)->num=++tn;
	(*(pb+i)).value=*pa++;
    }
    */
// 方式3--------------------------------------------
    /*
    for(;i<n;i++){
	pb->num=++tn;
	pb->value=*pa;
	pa++;
	pb++;
    }
    */
// 方式4--------------------------------------------
    for(;i<n;i++){
	pb[i].num=++tn;
	pb[i].value=pa[i];
    }
}

void put_st(struct st *pa,int n){
    int i=0;
    printf("put student information ----------\n\n");
// 方式1--------------------------------------------
/*
    for(;i<5;i++)
	printf("student %d : %s %d\n",i+1,(pa+i)->name,(pa+i)->age);
*/
//	printf("student %d : %s %d\n",i+1,(pa++)->name,(pa++)->age);//error
// 方式2--------------------------------------------
    for(;i<n;i++;pa++)
	printf("student %d : %s %d\n",i+1,pa->name,pa->age);
	
}

void put_te(struct te *pb,int n){
    int i=0;
    printf("put teacher information ----------\n\n");
// 方式1--------------------------------------------
/*
    for(;i<5;i++)
	printf("teachaer %d : %d %s %d\n",i+1,(pb+i)->num,(pb+i)->value.name,(pb+i)->value.age);
*/
// 方式2--------------------------------------------
    for(;i<n;i++;pb++){
	printf("teachaer %d : %d %s %d\n",i+1,pb->num,pb->value.name,pb->value.age);
    }

}

//结构体数组a接收键盘数据，结构体数组b从a获取，输出a和b内容
int main(){
    struct st a[5]={0};
    struct te b[5]={0};

    get_st(a,5);
    get_te(b,a,5);
    put_st(a,5);
    put_te(b,5);

    return 0;
}
