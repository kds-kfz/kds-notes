#include<stdio.h>
//n个人围成圈1,2,3循环数数，数到3的出局，问最后1个人是第几个？
int main(){
    int a1[3]={1,2,3};
    int i=0,j=0,k=0,l=0,num=0,h=0,flag=0,n=0,count=0;
    printf("请输入一个正数\n");
    scanf("%d",&n);
    int a2[n];
    int a3[n-1];
    while(num<n-1){
	num=0;
	k=0;
    for(i=0;i<n;i++){	
	if(flag==0){
	a2[i]=a1[j];
	j++;
	if(j==3)j=0;
	if(a2[i]==3){
	    a3[k]=i;k++;
	    num++;
	    k=num;
	}
	if(i==n-1){
	    flag=1;
	    count++;
	    printf("第%d次数数\n",count);
	    printf("需要出局的有\n");
	    for(l;l<k;l++)
		printf("%d ",a3[l]);
	    putchar(10);
	    continue;}
	}
	if(flag==1){
	    if(j==3){j=0;}
	    if(a2[i]!=3){
		a2[i]=a1[j];j++;
	    }
	    if(a2[i]==3){
		a3[k]=i;k++;
		num++;
		k=num;
	    }
	    if(i==n-1){
		count++;
		printf("第%d次数数\n",count);
		printf("需要出局的有\n");
		for(l=0;l<k;l++)
		    printf("%d ",a3[l]);
		putchar(10);
		}
	    }
	}
    }
    for(i=0;i<n;i++)
	printf("%d ",a2[i]);
    for(i=0;a2[i]==3;i++);
    putchar(012);
    printf("最后一个数是第%d个\n",i+1);

    return 0;
}
