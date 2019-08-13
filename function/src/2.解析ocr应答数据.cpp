#include <iostream>
#include <stdio.h>
#include <cstring>
#include <stdlib.h>

#include "../common/cJSON.h"

using namespace std;

int main()
{
	FILE* fp = fopen("../data/date_ocr_res_2.json","rb");
    if(fp == NULL){
        cout<<"目录不存在"<<endl;
        exit(1);
    }
	fseek(fp,0,SEEK_END);
	long int len = ftell(fp);
	rewind(fp);
	char c[len+1];
	fread(c,len,1,fp);
	c[len] = '\0';
	const char* s = c;
	
    char mesgbuff[2048] = {0};
    char *pbuff = mesgbuff;
    memcpy(pbuff, s, len + 1);
    string ocrmsg(mesgbuff);
    string::size_type npos1 = ocrmsg.find("{");
    string::size_type npos2 = ocrmsg.find_last_of("}");
    ocrmsg = ocrmsg.substr(npos1, npos2 - npos1 + 1);
    //printf("-->mesgbuff:\n%s\n",pbuff);
    //cout<<ocrmsg<<endl;
   
	cout<<"\n解析结果如下:"<<endl;
    cJSON* obj = cJSON_Parse(ocrmsg.c_str());
    cJSON* item = cJSON_GetObjectItem(obj,"RstCode");
    if(item){
        if(item->type == cJSON_String){
            cout<<"RstCode:"<<item->valuestring<<endl;
        }
    }

    item = cJSON_GetObjectItem(obj,"dataList");
    if(item){
        if(item->type == cJSON_Array){
            int size = cJSON_GetArraySize(item);
            for(int i = 0; i < size; i++){
                cJSON *arr = cJSON_GetArrayItem(item, i);
                if(arr){
                    //list数据解析
                    cJSON *obj_1 = cJSON_GetObjectItem(arr, "all_ocr");
                    if(obj_1){
                        if(obj_1->type == cJSON_Number){
                            cout<<"all_ocr:"<<obj_1->valueint<<endl;
                        }
                    }
                    obj_1 = cJSON_GetObjectItem(arr, "dataDataLen");
                    if(obj_1){
                        if(obj_1->type == cJSON_Number){
                            cout<<"dataDataLen:"<<obj_1->valueint<<endl;
                        }
                    }
                    obj_1 = cJSON_GetObjectItem(arr, "dataFormat");
                    if(obj_1){
                        if(obj_1->type == cJSON_Number){
                            cout<<"dataFormat:"<<obj_1->valueint<<endl;
                        }
                    }
                    obj_1 = cJSON_GetObjectItem(arr, "pattern_sn");
                    if(obj_1){
                        if(obj_1->type == cJSON_Number){
                            cout<<"pattern_sn:"<<obj_1->valueint<<endl;
                        }
                    }
                    obj_1 = cJSON_GetObjectItem(arr, "processRst");
                    if(obj_1){
                        if(obj_1->type == cJSON_Number){
                            cout<<"processRst:"<<obj_1->valueint<<endl;
                        }
                    }
                    obj_1 = cJSON_GetObjectItem(arr, "regionNum");
                    if(obj_1){
                        if(obj_1->type == cJSON_Number){
                            cout<<"pattern_sn:"<<obj_1->valueint<<endl;
                        }
                    }
                    obj_1 = cJSON_GetObjectItem(arr, "regions");
                    if(obj_1){
                        if(obj_1->type == cJSON_Array){
                            int size_1 = cJSON_GetArraySize(obj_1);
                            for(int i = 0; i < size_1; i++){
                                cJSON *arr_1 = cJSON_GetArrayItem(obj_1, i);
                                if(arr_1){
                                    //解析里层数组
                                    cJSON *obj_2 = cJSON_GetObjectItem(arr_1, "linNum");
                                    if(obj_2){
                                        if(obj_2->type == cJSON_Number){
                                            cout<<"linNum:"<<obj_2->valueint<<endl;
                                        }
                                    }
                                    obj_2 = cJSON_GetObjectItem(arr_1, "lines");
                                    if(obj_2){
                                        if(obj_2->type == cJSON_Array){
                                            int size_3 = cJSON_GetArraySize(obj_2);
                                            //这里先把记录保存起来，下面的动作是获取 content 的值
                                            //然后放到 容器 如果值唯一，则该条识别记录有效，否则过滤掉
                                            //char *p = cJSON_Print(obj_2);
                                            //printf("lines:%s\n", p);
                                            
                                            for(int k = 0; k < size_3; k++){
                                                cJSON *arr_2 = cJSON_GetArrayItem(obj_2, k);
                                                if(arr_2){
                                                    cJSON *obj_3 = cJSON_GetObjectItem(arr_2, "content");
                                                    if(obj_3){
                                                        if(obj_3->type == cJSON_String){
                                                            cout<<"content:"<<obj_3->valuestring<<endl;
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
        

//	printf ("\"%s\"\n",obj->valuestring);
//	printf("}\n");
	cJSON_Delete(obj);
	return 0;
}
