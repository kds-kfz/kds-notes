#include<stdio.h>
//6.c
//typedef声明新类型
/*
struct student{
    int num;
    char name[20];
    char sex;
    float score;
};
typedef struct student ST;

union mark{
    int num;
    char name[20];
};
typedef union mark MA;

enum weekday{
    mon,tue,wed,thu,fri,sat,sun
};
typedef enum  weekday WE;
*/
typedef struct{
    int num;
    char name[20];
    char sex;
    float score;
}ST;

typedef union{
    int num;
    char name[8];
}MA;

typedef enum{
    mon,tue,wed,thu,fri,sat,sun
}WE;
//typedef int * P;
typedef ST * P1;
typedef MA * P2;
typedef WE * P3;

int main(){

    ST s1={1001,"lisi",'M',89.78};
    MA m1={0};
    WE day;
    WE *p4=&day;
    
    P1 p1=&s1;
    P2 p2=&m1;
    P3 p3=&day;
    
    p2->num=65;

    printf("%d %s %c %.2f\n",p1->num,p1->name,p1->sex,p1->score);
    printf("%d %s\n",p2->num,p2->name);
//    printf("%d\n",*p3);

    for(;*p4<=6;(*p4)++){
	switch(*p4){
	    case mon : printf("mon\n");break;
	    case tue : printf("tue\n");break;
	    case wed : printf("wed\n");break;
	    case thu : printf("thu\n");break;
	    case fri : printf("fri\n");break;
	    case sat : printf("sat\n");break;
	    case sun : printf("sun\n");break;
	}
    }
    return 0;
}

