#include <stdio.h>
#include "cJSON.h"
int main()
{
	FILE* fp = fopen("4.json","rb");
	fseek(fp,0,SEEK_END);
	long int len = ftell(fp);
	rewind(fp);
	char a[len+1];
	fread(a,len,1,fp);
	a[len] = '\0';
	fclose(fp);
	const char* p = a;
	cJSON* obj = cJSON_Parse(p);
	if(obj)
	{
		if(obj->type == cJSON_Object)
		{
			printf("{\n\t");
			cJSON* item = cJSON_GetObjectItem(obj,"name");
			printf("\"name\":");
			if(item)
			{
				if(item->type == cJSON_String)
					printf("\"%s\",\n\t",item->valuestring);
			}
			item = cJSON_GetObjectItem(obj,"cities");
			printf("\"cities\":");
			if(item)
			{
				if(item->type == cJSON_Array)
				{
					printf("[");
					int size = cJSON_GetArraySize(item);
					int i = 0;
					for(;i<size;i++)
					{
						cJSON* obj1 = cJSON_GetArrayItem(item,i);
						if(obj1)
						{
							if(obj1 ->type == cJSON_Object)
							{
								printf("{\n\t\t");
								cJSON* o_item = cJSON_GetObjectItem(obj1,"省份");
								printf("\"省份\":");
								if(o_item)
								{
									if(o_item->type == cJSON_String)
										printf("\"%s\",\n\t\t",o_item->valuestring);
								}
								o_item = cJSON_GetObjectItem(obj1,"城市");
								printf("\"城市\":");
								if(o_item)
								{
									if(o_item->type == cJSON_Array)
									{
										printf("[");
										int size1 = cJSON_GetArraySize(o_item);
										int j = 0;
										for(;j<size1;j++)
										{
											cJSON* a_item = cJSON_GetArrayItem(o_item,j);
											if(a_item)
											{
												if(a_item->type == cJSON_String)
													printf("\"%s\"",a_item->valuestring);
											}

											if(j != size1-1)
												printf(",");
										}
										printf("],\n");

									}
								}
								printf("\t\t");
								o_item = cJSON_GetObjectItem(obj1,"邮编");
								printf("\"邮编\":");
								if(o_item)
								{
									if(o_item->type == cJSON_Number)
										printf("%d",o_item->valueint);
								}
								printf("}\n\t\t");

							}
						}

						if(i != size-1)
							printf(",");
					}
					printf("]\n");
					
				}
			}
			printf("}\n");
		}
	}
	return 0;
}
