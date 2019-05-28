#include <stdio.h>
#include <stdlib.h>
#include "json.h"
void print_array(JSON* json1);
int main()
{
	char a[2048] = "[\"ghj\",\"hg\"jh\"]";
	printf("输入:\n");
	fgets(a,1024,stdin);
//	scanf("%s",a);
	const char* p = a;
	JSON* json = Parse(p);
	if(json)
	{
		Print(json);
		printf("\n");
		Delete(json);


	}

	return 0;
}
