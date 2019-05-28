#include <stdio.h>
#include "cJSON.h"
int main()
{

	cJSON* arr = cJSON_CreateArray();

	cJSON* obj1 = cJSON_CreateObject();
	cJSON* obj2 = cJSON_CreateObject();

	cJSON_AddStringToObject(obj1,"城市","深圳");
	cJSON_AddStringToObject(obj1,"天气","台风");

	cJSON_AddStringToObject(obj2,"城市","广州");
	cJSON_AddStringToObject(obj2,"天气","晴天");

	cJSON_AddItemToArray(arr,obj1);
	cJSON_AddItemToArray(arr,obj2);

	char* p = cJSON_Print(arr);
	printf("%s\n",p);

printf("-------------------------\n");
	cJSON* a = cJSON_Parse(p);
	if(a)
	{
		if(a->type == cJSON_Array)
		{
			printf("[\n\t");
			int size = cJSON_GetArraySize(a);
			int i = 0;
			for(;i<size;i++)
			{
				cJSON* item = cJSON_GetArrayItem(a,i);
				if(item)
				{
					if(item->type == cJSON_Object)
					{
						printf("{\n\t\t");
						cJSON* o = cJSON_GetObjectItem(item,"城市");
						printf("城市:");
						if(o)
						{
							if(o->type == cJSON_String)
								printf("%s,\n\t\t",o->valuestring);
						}
						o = cJSON_GetObjectItem(item,"天气");
						printf("天气:");
						if(o)
						{
							if(o->type == cJSON_String)
								printf("%s\n",o->valuestring);
						}
					printf("}");
					}
				}
				if(i != size-1)
					printf(",\n");
				else
					printf("\n");
			}
			printf("]\n");

		}

	}



}
