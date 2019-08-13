#include <stdio.h>
#include<string.h>
#include "cJSON.h"
int main()
{
	cJSON* obj = cJSON_CreateObject();
	cJSON* arr = cJSON_CreateArray();
	cJSON* obj1 = cJSON_CreateObject();
	cJSON_AddStringToObject(obj1,"省份","江苏");
	cJSON_AddItemToArray(arr,obj1);
	
    cJSON_AddItemToObject(obj,"cities",arr);
	char* p = cJSON_Print(obj);
	printf("%s\n",p);

    return 0;
}
