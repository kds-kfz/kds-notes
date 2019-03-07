#include<stdio.h>

union st{
    char a;
    short b;
    int c;
    float d;
    double e;
};

//无名共用体
union {
    char a;
    short b;
    int c;
    float d;
    double e;
}st_2;

typedef enum {
    Mon,Tue,Wed,Thu,Fri,Sat,Sun
}Weeken;

int main(){
    union st st_1;
    //sizeof st_1 = 8, sizeof st_2 = 8
    printf("sizeof st_1 = %d, sizeof st_2 = %d\n", sizeof(st_1), sizeof(st_2));
    
    st_1.a = '9';
    //9, 39, 39, 0.000000, 0.000000
    printf("%c, %x, %X, %f, %lf\n", st_1.a, st_1.b, st_1.c, st_1.d, st_1.e);
    
    //st_1.c = 0x3E25AD01;
    st_1.c = 0x3E25AD54;
    //st_1.c = 0x04030201;
    char c = (char)st_1.c;//84
    //T, ffffad54, 3E25AD54, 0.161794, 0.000000, 84
    printf("%c, %x, %X, %f, %lf, %d\n", st_1.a, st_1.b, st_1.c, st_1.d, st_1.e, c);
    
    Weeken day, *p = &day;
    do{
        switch(*p){
            case Mon: printf("Mon = %d\n", day);break;
            case Tue: printf("Tue = %d\n", day);break;
            case Wed: printf("Wed = %d\n", day);break;
            case Thu: printf("Thu = %d\n", day);break;
            case Fri: printf("Fri = %d\n", day);break;
            case Sat: printf("Sat = %d\n", day);break;
            case Sun: printf("Sun = %d\n", day);break;
        }
    }while((*p)++ < 7);

    return 0;
}
