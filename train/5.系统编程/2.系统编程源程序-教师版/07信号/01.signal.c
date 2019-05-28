#include <stdio.h>

typedef void(*FUN)(int);

struct per{
	FUN fun;
	int id;
};	

void police(int num)
{
	printf("police:%d\n",num);
	return;
}

void fire(int num)
{
	printf("fire:%d\n",num);
	return;
}

void ambulan(int num)
{
	printf("ambulan:%d",num);
	return;
}

int main()
{
	int i;
	int num;
	struct per p[3] = {{police,110},{fire,119},{ambulan,120}};
	printf("input your sig:\n");
	scanf("%d",&num);
	for(i=0; i<3; i++){
		if(num == p[i].id)
			p[i].fun(num);
	}
	if(i > 3)
		printf("input error...\n");

	return;
}
