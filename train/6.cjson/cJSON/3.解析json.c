#include <stdio.h>
#include "cJSON.h"
void print_array(cJSON* json1)
{
	cJSON* json = json1->child;
	if (json)
	{
		printf ("[");
		while(json)
		{
			if(json->type == cJSON_String)
				printf("\"%s\"",json->valuestring);
			else if(json->type == cJSON_NULL)
				printf("null");
			else if (json->type == cJSON_False)
				printf("false");
			else if (json->type == cJSON_True)
				printf("true");
			else if (json->type == cJSON_Number)
				printf("%d",json->valueint);
			else if (json->type == cJSON_Array)
				print_array(json);
			json = json->next;
			if (json)
				printf(",");
		}
		printf ("]");
	}
	else
	{
		printf ("空数组\n");
	}
}
int main()
{
	FILE* fp = fopen("4.json","rb");
	fseek(fp,0,SEEK_END);
	long int len =  ftell(fp);
	rewind(fp);
	char a[len+1];
	fread(a,len,1,fp);
	a[len] = '\0';
	fclose(fp);
	const char* p = a;
cJSON* json = cJSON_Parse(p);
if (json)
{
	if(json->type == cJSON_Array)
	{
		printf ("[ ");
		int size = cJSON_GetArraySize(json);
		int i = 0;
		for(;i < size;i++)
		{
			cJSON* item = cJSON_GetArrayItem(json,i);
			if(item)
			{
				if(item->type == cJSON_Array)
				{
					printf("[");
					int size1 = cJSON_GetArraySize(item);
					int j = 0;
					for(;j < size1 ;j++)
					{
						cJSON* item1 = cJSON_GetArrayItem(item,j);
						if(item1)
					{
						if(item1 ->type == cJSON_Number)
						{
							if (i == 0)
								printf("%d ",item1->valueint);
							else 
								printf ("%.2f ",item1->valuedouble);
						}
						else if (item1 ->type == cJSON_String)
							printf("%s  ",item1->valuestring);
						else if (item1 ->type == cJSON_NULL)
							printf("null");
						else if(item1->type == cJSON_False)
							printf("false");
						else if (item1 ->type == cJSON_True)
							printf("true");
						}
						if(j != size1-1)
							printf(",");
					}
						printf("]");
				}
			}
			if (i != size-1 )
				printf (",");
		}
			printf ("]\n");
	}
}
			
	cJSON_Delete(json);
}
