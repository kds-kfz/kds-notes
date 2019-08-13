#include<iostream>
#include <stdio.h>
#include<string.h>
#include "cJSON.h"

using namespace std;
int main()
{
	cJSON* obj = cJSON_CreateObject();
	cJSON* arr = cJSON_CreateArray();
	cJSON* obj1 = cJSON_CreateObject();
	cJSON_AddStringToObject(obj1,"省份","江苏");
	cJSON_AddItemToArray(arr,obj1);
	
    //cJSON_AddItemToObject(obj,"cities",arr);
	//char* p = cJSON_Print(obj);
	char* p = cJSON_Print(arr);
	printf("%s\n",p);
    const char* p2 = "[{\"name\":\"zhangsan\"}]";
    cJSON*  json = cJSON_Parse(p2);
    if(json->type == cJSON_Array){
        int size = cJSON_GetArraySize(json);
        int i = 0;
        for(;i<size;i++){
            //这里是同级对象获取
            cJSON* obj1 = cJSON_GetArrayItem(json,i);
            if(obj1){
                if(obj1 ->type == cJSON_Object){
                    cJSON* o_item = cJSON_GetObjectItem(obj1,"name");
                    if(o_item){
                        printf("%s:","name");
                        if(o_item->type == cJSON_String){
                            printf("%s\n",o_item->valuestring);
                        }
                    }
                }
            }
        }
    }
	//printf("%s\n",p2);
    //数组中插入对象
    cJSON* arr1 = cJSON_CreateArray();
    cJSON* obj2 = cJSON_CreateObject();
    cJSON_AddStringToObject(obj2,"省份","广西");
    cJSON_AddItemToArray(arr1,obj2);
    char* p3 = cJSON_Print(arr);
    printf("%s\n",p3);

    //将json数据转成字符串
    string tmp(p3);
    cout<<"--p3:"<<tmp<<endl;

    //解析字符串中的json数据
    cJSON*  json4 = cJSON_Parse(tmp.c_str());
    if(json4->type == cJSON_Array){
        int size = cJSON_GetArraySize(json4);
        int i = 0;
        for(;i<size;i++){
            //这里是同级对象获取
            cJSON* arry = cJSON_GetArrayItem(json4,i);
            if(arry){
                if(arry->type == cJSON_Object){
                    cJSON* o_item = cJSON_GetObjectItem(arry,"省份");
                    if(o_item){
                        printf("%s:","省份");
                        if(o_item->type == cJSON_String){
                            printf("%s\n",o_item->valuestring);
                        }
                    }
                }
            }
        }
    }
    return 0;
}
