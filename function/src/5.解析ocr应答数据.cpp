#include <iostream>
#include <stdio.h>
#include <cstring>
#include <stdlib.h>
#include <vector>
#include <map>

#include "../common/cJSON.h"

using namespace std;

bool MapAgin(map<int, string> &dest, map<int, string> &src){
    if(!src.empty()){
        map<int, string>::iterator SrcItem, DestItem;
        if(dest.empty()){
            for(SrcItem = src.begin(); SrcItem != src.end(); SrcItem++){
                dest.insert(make_pair((*SrcItem).first, (*SrcItem).second));
            }
        }else{
            DestItem = dest.end();
            DestItem--;
            int key = (*DestItem).first;
            for(SrcItem = src.begin(); SrcItem != src.end(); SrcItem++){
                cJSON* obj = cJSON_Parse((*SrcItem).second.c_str());
                //是个对象
                if(obj){
                    key += 1;
                    cJSON *obj_1 = cJSON_GetObjectItem(obj, "lines");
                    if(obj_1){
                        if(obj_1->type == cJSON_Array){
                            int size = cJSON_GetArraySize(obj_1);
                            for(int i = 0; i < size; i++){
                                cJSON *arr = cJSON_GetArrayItem(obj_1, i);
                                if(arr){
                                    cJSON *obj_2 = cJSON_GetObjectItem(arr, "sn_Inner");
                                    if(obj_2){
                                        if(obj_2->type == cJSON_Number){
                                            //替换
                                            cJSON_ReplaceItemInObject(arr,"sn_Inner",cJSON_CreateNumber(key));
                                        }
                                        //删除，清理空间，再添加新值
                                        //cJSON_DeleteItemFromObject(arr, "sn_Inner");
                                        //cJSON_AddNumberToObject(arr,"sn_Inner", key);
                                    }
                                }
                            }
                        }
                    }
                char *p = cJSON_Print(obj);
                //cout<<key<<":"<<p<<endl;
                string newvalue(cJSON_Print(obj));
                dest.insert(make_pair(key, newvalue));
	            cJSON_Delete(obj);
                free(p);
                }
            }
        }
        return true;
    }else if(dest.empty()){//第二次进入该函数 dest 容器不能为空
        return false;
    }
    return true;
}

void ParseOcr(const char *patch, map<int, string> &data){
	FILE* fp = fopen(patch, "rb");
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
	
    char mesgbuff[102400] = {0};
    char *pbuff = mesgbuff;
    memcpy(pbuff, s, len + 1);
    string ocrmsg(mesgbuff);
    string::size_type npos1 = ocrmsg.find("{");
    string::size_type npos2 = ocrmsg.find_last_of("}");
    ocrmsg = ocrmsg.substr(npos1, npos2 - npos1 + 1);
    //printf("-->mesgbuff:\n%s\n",pbuff);
    //cout<<ocrmsg<<endl;
   
	//cout<<"\n解析结果如下:"<<endl;
    cJSON* obj = cJSON_Parse(ocrmsg.c_str());
    cJSON* item = NULL;
    item = cJSON_GetObjectItem(obj,"dataList");
    if(item){
        if(item->type == cJSON_Array){
            int size = cJSON_GetArraySize(item);
            for(int i = 0; i < size; i++){
                cJSON *arr = cJSON_GetArrayItem(item, i);
                if(arr){
                    cJSON *obj_1 = NULL;
                    obj_1 = cJSON_GetObjectItem(arr, "regions");
                    if(obj_1){
                        if(obj_1->type == cJSON_Array){
                            int size_1 = cJSON_GetArraySize(obj_1);
                            for(int i = 0; i < size_1; i++){
                                cJSON *arr_1 = cJSON_GetArrayItem(obj_1, i);
                                if(arr_1){
                                    //保存整个数据
                                    char *p = cJSON_Print(arr_1);
                                    cJSON *obj_2 = NULL;
                                    obj_2 = cJSON_GetObjectItem(arr_1, "lines");
                                    if(obj_2){
                                        if(obj_2->type == cJSON_Array){
                                            int size_3 = cJSON_GetArraySize(obj_2);
                                            for(int k = 0; k < size_3; k++){
                                                cJSON *arr_2 = cJSON_GetArrayItem(obj_2, k);
                                                if(arr_2){
                                                    cJSON *obj_3 = cJSON_GetObjectItem(arr_2, "sn_Inner");
                                                    if(obj_3){
                                                        if(obj_3->type == cJSON_Number){
                                                            //cout<<"sn_Inner:"<<obj_3->valueint<<endl;
                                                            //printf("[%d]:[%s]\n", obj_3->valueint, p);
                                                            char temp[2028] = {0};
                                                            sprintf(temp, "%s", p);
                                                            data.insert(make_pair(obj_3->valueint, string(p)));
                                                        }
                                                    }
                                                }
                                            }
                                        }
                                    }
                                    free(p);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
	cJSON_Delete(obj);
    fclose(fp);
}


void MakeOcrRes(const char *destpatch, const char *srcpatch, map<int, string> &data){
	FILE* fp = fopen(srcpatch, "rb");
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
	
    char mesgbuff[102400] = {0};
    char *pbuff = mesgbuff;
    memcpy(pbuff, s, len + 1);
    string ocrmsg(mesgbuff);
    string::size_type npos1 = ocrmsg.find("{");
    string::size_type npos2 = ocrmsg.find_last_of("}");
    ocrmsg = ocrmsg.substr(npos1, npos2 - npos1 + 1);
    //printf("-->mesgbuff:\n%s\n",pbuff);
    cout<<ocrmsg<<endl;
   
	//cout<<"\n解析结果如下:"<<endl;
    int Num = data.size();
    cJSON* obj = cJSON_Parse(ocrmsg.c_str());
    cJSON* item = NULL;
    item = cJSON_GetObjectItem(obj,"dataList");
    if(item){
        if(item->type == cJSON_Array){
            int size = cJSON_GetArraySize(item);
            for(int i = 0; i < size; i++){
                cJSON *arr = cJSON_GetArrayItem(item, i);
                if(arr){
                    cJSON *obj_1 = NULL;
                    obj_1 = cJSON_GetObjectItem(arr, "regionNum");
                    if(obj_1){
                        if(obj_1->type == cJSON_Number){
                            //替换该值
                            //cout<<"pattern_sn:"<<obj_1->valueint<<endl;
                            cJSON_ReplaceItemInObject(arr,"regionNum", cJSON_CreateNumber(Num));
                        }
                    }
                    //删除 regions 的值，并释放空间
                    obj_1 = cJSON_GetObjectItem(arr, "regions");
                    int size_1 = 0;
                    while(size_1 = cJSON_GetArraySize(obj_1)){
                        if(obj_1){
                            if(obj_1->type == cJSON_Array){
                                for(int i = 0; i < size_1; i++){
                                    cJSON_DeleteItemFromArray(obj_1, i);
                                }
                            }
                        }
                    }
                    //建立新值
                    cJSON* obj2 = NULL;
                    cout<<"data,size = "<<data.size()<<endl;
                    for(map<int, string>::iterator it = data.begin(); it != data.end(); it++){
                        cout<<"建立新值:"<<(*it).second.c_str()<<endl;
                        obj2 = cJSON_Parse((*it).second.c_str());
                        cJSON_AddItemToArray(obj_1, obj2);
                    }
                }
            }
        }
    }
    char* p2 = cJSON_Print(obj);
	printf ("%s\n",p2);
	FILE* fp2 = fopen(destpatch, "wb");
	if(fp2 == NULL){
        cout<<"目录不存在"<<endl;
        exit(1);
    }
    fwrite(p2, strlen(p2), 1, fp2);
	fclose(fp2);

    //这里释放外层的cJSON *指针即可
    free(p2);
	
    cJSON_Delete(obj);
    fclose(fp);
}


int main()
{
    map<int, string> LastMap, NowMap;
    vector< map<int, string> > TotalVector;

    //解析 json 数据到容器
    ParseOcr("../data/date_ocr_res_2.json", LastMap);
    ParseOcr("../data/date_ocr_res_3.json", NowMap);
        
    //指定起始 sn_Inner 号，并插入旧容器
    if(MapAgin(LastMap, NowMap)){
        /*
        map<int, string>::iterator it = LastMap.begin();
        for(;it != LastMap.end(); it ++){
            //cout<<(*it).first<<endl;
            cout<<(*it).second<<endl;
        }
        */
        //重新生成应答数据，更新原来生成的应答数据文件
        MakeOcrRes("../data/date_ocr_res_4.json", "../data/date_ocr_res_2.json", LastMap);
    }
    
	return 0;
}
