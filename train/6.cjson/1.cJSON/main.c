#include <stdio.h>
#include "cJSON.h"
int main()
{
	const char* p = "[1,2,3,[],4]";

	cJSON* json = cJSON_Parse(p);
	const char* s = cJSON_Print(json);
	printf("s = %s\n",s);
	return 0;
}
