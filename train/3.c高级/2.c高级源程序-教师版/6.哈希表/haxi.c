#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct st{
    int year;
    char name[20];
    struct st* next;
}Data;
Data* arr[10] = {NULL};
int fun(int n)
{
    return n % 10;
}
void creat(Data d1)
{
    int i = fun(d1.year);
    Data* p = (Data*)malloc(sizeof(Data));
    p->year = d1.year;
    strcpy(p->name,d1.name);
    p->next = NULL;
    if (arr[i] == NULL)
	arr[i] = p;
    else{
	Data* tail = arr[i];
	while(tail->next != NULL)
	    tail = tail->next;
	tail->next = p;
    }
}

void print_all()
{
    int i = 0;
    for(;i<10;i++)
    {
	printf ("%d里面存的数据有:\n",i);
	Data* h = arr[i];
	while(h != NULL)
	{
	    printf ("%d %s\n",h->year,h->name);
	    h = h->next;
	}
    }
}
void search_year(int n)
{
    int i = fun(n);
    Data * h = arr[i];
    printf ("%d年的有\n",n);
    while(h != NULL)
    {
	printf ("%d %s\n",h->year,h->name);
	h = h->next;
    }
}
void search_name(char* s)
{
    int i = 0;
    int count = 0;
    int flag = 0;
    for(;i<10;i++)
    {
	Data* h = arr[i];
	count = 0;
	while (h != NULL)
	{
	    count++;
	    if(strcmp(h->name,s)== 0)
	    {
	    printf("下标为%d第%d个%d %s\n",i,count,h->year,h->name);
	    flag = 1;
	    }
	    h = h->next;
	}
    }
    if (flag == 0)
	printf ("不存在\n");
}

void free_all()
{
    int i = 0;
    Data* h = NULL;
    for(;i<10;i++)
    {
	h =  arr[i];
	while(h != NULL)
	{
	    Data* s = h;
	    h = h->next;
	    free(s);
	}
    }
}

int main()
{
    system("clear");
    Data d1 = {1990,"zhang"};
    Data d2 = {1991,"wang"};
    Data d3 = {1992,"li"};
    Data d4 = {1993,"zh"};
    Data d5 = {1994,"ab"};
    Data d6 = {1995,"abc"};
    Data d7 = {1996,"abcd"};
    Data d8 = {1997,"zhao"};
    Data d9 = {1998,"qian"};
    Data d10 = {1999,"sun"};

    Data d11 = {1994,"zhang"};
    Data d12 = {1994,"wang"};
    Data d13 = {1994,"zhe"};
    Data d14 = {1995,"asd"};
    Data d15 = {1990,"zg"};
    Data d16 = {1999,"zhan"};

    creat(d1);
    creat(d2);
    creat(d3);
    creat(d4);
    creat(d5);
    creat(d6);
    creat(d7);
    creat(d8);
    creat(d9);
    creat(d10);
    creat(d11);
    creat(d12);
    creat(d13);
    creat(d14);
    creat(d15);
    creat(d16);
    int n = 0;
    printf ("输入年份\n");
    scanf("%d",&n);
    search_year(n);

    char name[20];
    printf ("输入姓名\n");
    scanf("%s",name);
    search_name(name);



    print_all();
    free_all();

    print_all();
}
