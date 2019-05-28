#include<stdio.h>
#include<string.h>
//6.c
//结构体嵌套

struct st{
    char name[20];
    int age;
};
struct te{
    struct st value;
    int num;
};

void put_ste(struct st b[],struct te a[]){
    int i;
    for(i=0;i<3;i++){
	strcpy(b[i].name,a[i].value.name);
	b[i].age=a[i].value.age;
    }

    printf("printf struct b[3]-------------------\n");
    for(i=0;i<3;i++)
	printf("%s %d\n",b[i].name,b[i].age);
}

int main(){
    /*
       struct st s1={"lisi",24};
       struct st s2={"lisi",24};
       struct st s3={"lisi",24};

       struct te t1={s1,1001};
       struct te t2={"liwu",25,1001};
       struct te t3={.value.name="lier",
       .value.age=25,
       .num=1002};

       printf("%s %d\n",s1.name,s1.age);
       printf("%s %d %d\n",t3.value.name,t3.value.age,t3.num);
     */
    //练习：输入结构体a[3],结构体b[3]从a[3]获取，打印b[3]
    int i;

    struct te a[3];
    struct st b[3];

    printf("input  struct a[3]-------------------\n");
    for(i=0;i<3;i++)
	scanf("%s %d %d",a[i].value.name,&a[i].value.age,&a[i].num);
    /*
       for(i=0;i<3;i++){
       strcpy(b[i].name,a[i].value.name);
       b[i].age=a[i].value.age;
       }

       printf("printf struct b[3]-------------------\n");
       for(i=0;i<3;i++)
       printf("%s %d\n",b[i].name,b[i].age);
     */
    put_ste(b,a);
    return 0;
}
