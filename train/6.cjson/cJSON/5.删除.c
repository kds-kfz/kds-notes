//cJSON_DeleteItemFromArray(cJSON *array,int which);
//cJSON_DeleteItemFromObject(cJSON *object,const char *string);
//cJSON_ReplaceItemInArray(cJSON *array,int which,cJSON *newitem);
//cJSON_ReplaceItemInObject(cJSON *object,const char *string,cJSON *newitem); 

#include <stdio.h>
#include "cJSON.h"
int main()
{
	cJSON* arr = cJSON_CreateArray();
	cJSON_AddItemToArray(arr,cJSON_CreateString("aaa"));
	cJSON_AddItemToArray(arr,cJSON_CreateNumber(18));

	cJSON* obj = cJSON_CreateObject();
	cJSON_AddStringToObject(obj,"name","wang");

	cJSON_AddItemToArray(arr,obj);

	cJSON* item = cJSON_CreateString("hhhhhhhhhhhhhhh");
	cJSON* item1 = cJSON_CreateString("hhhhhhhhhhhhhhh");

	cJSON_ReplaceItemInArray(arr,0,item);
	cJSON_ReplaceItemInArray(arr,1,item1);
//	cJSON_DeleteItemFromArray(arr,2);

	char* p = cJSON_Print(arr);
	printf("p = %s\n",p);

	
	
	cJSON* obj1 = cJSON_CreateObject();
	cJSON_AddStringToObject(obj1,"name","wang");
	cJSON_AddNumberToObject(obj1,"age",18);
	cJSON_AddItemToObject(obj1,"hhh",arr);
//	cJSON_DeleteItemFromObject(obj1,"hhh");
	int a[5] = {1,2,3,4,5};
	cJSON* arr1 = cJSON_CreateIntArray(a,5);
	cJSON_ReplaceItemInObject(obj1,"name",arr1);

	p = cJSON_Print(obj1);
	printf("%s\n",p);






}
