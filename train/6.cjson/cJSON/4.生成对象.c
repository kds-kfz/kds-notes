#include <stdio.h>
#include "cJSON.h"
#include <string.h>
int main()
{
	cJSON* obj = cJSON_CreateObject();
	printf("obj->child:%p\n",obj->child);
	printf ("obj->string:%s\n",obj->string);

	cJSON* item1 = cJSON_CreateString("zhangsan");
	cJSON_AddItemToObject(obj,"name",item1);

	printf("item1:%p\n",item1);
	printf("obj->child:%p\n",obj->child);
	printf ("item1->string:%s\n",item1->string);
	printf ("item1->valuestring:%s\n",item1->valuestring);


	cJSON_AddNumberToObject(obj,"age",18);
	printf("item1->next->string:%s\n",item1->next->string);
	printf("item1->next->valueint:%d\n",item1->next->valueint);

	cJSON_AddNullToObject(obj,"car");
	cJSON_AddNumberToObject(obj,"score",78);
	cJSON_AddStringToObject(obj,"学校","蓝翔");

	const char* a[3] = {"挖掘机","厨师","数控"};
	cJSON* arr = cJSON_CreateStringArray(a,3);
	cJSON_AddItemToObject(obj,"专业",arr);

	char* p = cJSON_Print(obj);
	printf ("%s\n",p);
	FILE* fp = fopen("4.json","wb");
	fwrite(p,strlen(p),1,fp);
	fclose(fp);
	cJSON_Delete(obj);
	return 0;
}
