#include <stdio.h>
#include "cJSON.h"
#include <string.h>
#include <stdlib.h>
int main()
{
	FILE* fp = fopen("4.json","rb");
	fseek(fp,0,SEEK_END);
	long int len = ftell(fp);
	rewind(fp);
	char c[len+1];
	fread(c,len,1,fp);
	c[len] = '\0';
	const char* s = c;
	cJSON* obj = cJSON_Parse(s);
	printf("{\n\t");
	cJSON* item = cJSON_GetObjectItem(obj,"name");
	printf ("\"name\":");
	if (item)
	{
		if(item->type == cJSON_String)
		{
		//	char* p = (char*)malloc(strlen(item->valuestring));
		//	strcpy(p,item->valuestring);
			printf ("\"%s\"\n",item->valuestring);
	//		printf ("%s\n",p);
		}
	}

	item = cJSON_GetObjectItem(obj,"age");
	printf ("\t\"age\":");
	if(item)
	{
		if (item->type == cJSON_Number)
		{
			printf("%d\n",item->valueint);
		}
	}

	item = cJSON_GetObjectItem(obj,"专业");
	printf ("\t\"专业\":");
	if (item)
	{
		if(item->type == cJSON_Array)
		{
			printf("[");
			int size = cJSON_GetArraySize(item);
			int i = 0;
			for(;i < size ;i++)
			{
				cJSON* arr = cJSON_GetArrayItem(item,i);
				if(arr)
				{
					if(arr->type == cJSON_String)
						printf("\"%s\" ",arr->valuestring);
				}
				if(i != size-1)
					printf(",");
			}
			printf("]\n");

		}
	}
	printf("}\n");
	cJSON_Delete(obj);
	return 0;
}
