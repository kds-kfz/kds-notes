#include <stdio.h>
#include<string.h>
#include "cJSON.h"
int main()
{

	cJSON* obj = cJSON_CreateObject();
	cJSON_AddStringToObject(obj,"name","中国");
	cJSON* arr = cJSON_CreateArray();

	cJSON* obj1 = cJSON_CreateObject();
	cJSON* obj2 = cJSON_CreateObject();
	cJSON* obj3 = cJSON_CreateObject();

	const char* a[2] = {"南京","苏州"};
	cJSON* a1 = cJSON_CreateStringArray(a,2);

	const char* b[3] = {"深圳","广州","佛山"};
	cJSON* b1 = cJSON_CreateStringArray(b,3);

	const char* c[2] = {"武汉","宜昌"};
	cJSON* c1 = cJSON_CreateStringArray(c,2);

	cJSON_AddStringToObject(obj1,"省份","江苏");
	cJSON_AddItemToObject(obj1,"城市",a1);
	cJSON_AddNumberToObject(obj1,"邮编",400);

	cJSON_AddStringToObject(obj2,"省份","广东");
	cJSON_AddItemToObject(obj2,"城市",b1);
	cJSON_AddNumberToObject(obj2,"邮编",4001);

	cJSON_AddStringToObject(obj3,"省份","湖北");
	cJSON_AddItemToObject(obj3,"城市",c1);
	cJSON_AddNumberToObject(obj3,"邮编",4002);

	cJSON_AddItemToArray(arr,obj1);
	cJSON_AddItemToArray(arr,obj2);
	cJSON_AddItemToArray(arr,obj3);
	cJSON_AddItemToObject(obj,"cities",arr);

	char* p = cJSON_Print(obj);
	printf("%s\n",p);

	FILE* fp = fopen("4.json","wb");
	fwrite(p,strlen(p),1,fp);
	fclose(fp);
	cJSON_Delete(obj);



	return 0;
}
