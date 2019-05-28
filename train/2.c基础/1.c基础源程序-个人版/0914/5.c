#include<stdio.h>
//5.c
//枚举类型
enum mei{
    mon,tue,wed,thu,fri,sat,sun
};

int main(){
/*    
    switch(day){
	case mon:printf("mon=%d\n",1);break;
    
    }
    */
    enum mei day;
//    for(day=mon;day<=sun;day++){
    for(day=0;day<=6;day++){
	switch(day){
	    case mon:printf("mon\n");break;
	    case tue:printf("tue\n");break;
	    case wed:printf("wed\n");break;
	    case thu:printf("thu\n");break;
	    case fri:printf("fri\n");break;
	    case sat:printf("sat\n");break;
	    case sun:printf("sun\n");break;
	    default :break;
	}
    }
    putchar(10);
    return 0;
}
