#include"stdio.h"
//2.c

int main(){
int a=0;
while(a<10)
{
    if(a==5)
    goto loop;
    printf("%d\n",a);
    loop:
    a++;
}
    return 0;
}


int main1(){

int i=0,sum=0;
while(i<10){
i++;
    if(i%3==0)
	continue;
    sum+=i;
}
printf("sum=%d\n",sum);



    
return 0;
}
