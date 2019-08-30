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
    }else{ 
        if(dest.empty()){//第二次进入该函数 dest 容器不能为空
            return false;
        }
        //第二次进入该函数 src 容器不能为空
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
    data.clear();

	//cout<<"\n解析结果如下:"<<endl;
    cJSON* obj = cJSON_Parse(ocrmsg.c_str());
    cJSON* item = NULL;

    item = cJSON_GetObjectItem(obj, "RstMesg");
    if(item){
        if(item->type == cJSON_String){
        //    cout<<"RstMesg:"<<item->valuestring<<endl;
        }
    }

    cJSON *objs = cJSON_Parse(item->valuestring);
    if(objs->type == cJSON_Array){
        int sizes = cJSON_GetArraySize(objs);
        for(int a = 0; a < sizes; a++){
            cJSON *arrs = cJSON_GetArrayItem(objs, a);
            if(arrs){
                item = cJSON_GetObjectItem(arrs,"dataList");
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
            }
        }
    }
	cJSON_Delete(obj);
    fclose(fp);
}
#if 0
{
    "RstCode":  "0000",
    "RstMesg":  "[{\n\t\"dataList\":\t[{\n\t\t\t\"all_ocr\":\t0,\n\t\t\t\"dataDataLen\":\t0,\n\t\t\t\"dataFormat\":\t0,\n\t\t\t\"pattern_sn\":\t0,\n\t\t\t\
"processRst\":\t0,\n\t\t\t\"regionNum\":\t0,\n\t\t\t\"regions\":\tnull,\n\t\t\t\"dataNum\":\t1\n\t\t}]\n}]",
    "IDFilewPath":  null,
    "HeadFilePath": null,
    "filename": null
}
#endif

void MakeOcrRes(const char *destpatch, map<int, string> &data){
    //公共应答部分一
    cJSON* obj = cJSON_CreateObject();
    cJSON_AddNumberToObject(obj,"all_ocr",0);
    cJSON_AddNumberToObject(obj,"dataDataLen",0);
    cJSON_AddNumberToObject(obj,"dataFormat",0);
    cJSON_AddNumberToObject(obj,"dataIndex",0);

    int Num = 0;
    if(!data.empty()){
        Num = data.size();
        cJSON_AddNumberToObject(obj,"pattern_sn",1001);
        cJSON_AddNumberToObject(obj,"processRst",-1268136496);
        cJSON_AddNumberToObject(obj,"regionNum",Num);
        
        //对象中添加对象，对象值为 map 的值
        //cJSON_AddNullToObject(obj, "regions");
        cJSON *arr1 = cJSON_CreateArray();
        map<int, string>::iterator it = data.begin();
        for(; it != data.end(); it++){
            cJSON *obj1 = cJSON_Parse(it->second.c_str());
            //向数组中添加对象
            cJSON_AddItemToArray(arr1, obj1);
        }
        
        //对象中添加对象
        cJSON_AddItemToObject(obj, "regions", arr1);


    }else{
        //重新生成应答数据，如果没有任何切片识别成功，则默认返回空
        
        //在此对象中添加剩余对象
    	cJSON_AddNumberToObject(obj,"pattern_sn",0);
    	cJSON_AddNumberToObject(obj,"processRst",0);
        cJSON_AddNumberToObject(obj,"regionNum",Num);
        
        //对象中添加对象，对象值为空
        cJSON_AddNullToObject(obj, "regions");
    }

    //公共应答部分二
    //向数组中添加对象
    //dataList 对象值是个数组 arr2
    cJSON* arr2 = cJSON_CreateArray();
    cJSON_AddItemToArray(arr2, obj);

    //cJSON *obj2 = cJSON_CreateObject();
    //cJSON_AddItemToObject(obj2, "dataList", arr2);

    cJSON* obj_3 = cJSON_CreateObject();
    //向对象中插入对象
    cJSON_AddStringToObject(obj_3,"RstCode","0000");
    //对象中添加对象
    
    //创建 dataList 对象
    cJSON* obj_4 = cJSON_CreateObject();
    cJSON_AddItemToObject(obj_4, "dataList", arr2);
    //向对象中插入对象
    cJSON_AddNumberToObject(obj_4,"dataNum",1);
    char *p = cJSON_Print(obj_4);
    string temp;
    temp = "[" + string(p) + "]";
    
    cJSON_AddStringToObject(obj_3,"RstMesg", temp.c_str());
    
    cJSON_AddNullToObject(obj_3,"IDFilewPath");
    cJSON_AddNullToObject(obj_3,"HeadFilePath");
    cJSON_AddNullToObject(obj_3,"filename");
    
    //将新应答写到文件
    char *p2 = cJSON_Print(obj_3);
    cout<<"\n~~~新应答数据:\n"<<p2<<endl;
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
}

//该函数 有个bug，基数据必须是一个非空的应答数据，而且 regions 非空值
//MakeOcrRes("../data/date_ocr_res_4.json", "../data/date_ocr_res_3.json", TotalMap);
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
    //cout<<ocrmsg<<endl;
   
	//cout<<"\n解析结果如下:"<<endl;
    int Num = data.size();
    cout<<"~~~Num:"<<Num<<endl;
    cJSON* obj = cJSON_Parse(ocrmsg.c_str());
    cJSON* item = NULL;

    item = cJSON_GetObjectItem(obj, "RstMesg");
    if(item){
        if(item->type == cJSON_String){
        //    cout<<"RstMesg:"<<item->valuestring<<endl;
        }
    }

    cJSON *objs = cJSON_Parse(item->valuestring);
    if(objs->type == cJSON_Array){
        int sizes = cJSON_GetArraySize(objs);
        for(int a = 0; a < sizes; a++){
            cJSON *arrs = cJSON_GetArrayItem(objs, a);
            if(arrs){
                item = cJSON_GetObjectItem(arrs,"dataList");
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
            }
        }
    }
    //替换 RstMesg 的值
    char* p2 = cJSON_Print(objs);
	printf ("\n~~~~~%s\n",p2);
    cJSON_ReplaceItemInObject(obj,"RstMesg", cJSON_CreateString(p2));
    //释放 RstMesg 值内存
    free(p2);
    //重新解析新的值
    p2 = cJSON_Print(obj);
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
    map<int, string> TotalMap, NewMap;

    //解析 json 数据到容器
    ParseOcr("../data/date_ocr_res_5.json", NewMap);
    MapAgin(TotalMap, NewMap);
    ParseOcr("../data/date_ocr_res_2.json", NewMap);
    MapAgin(TotalMap, NewMap);
    ParseOcr("../data/date_ocr_res_3.json", NewMap);
    MapAgin(TotalMap, NewMap);
    
    MakeOcrRes("../data/date_ocr_res_4.json", TotalMap);   
#endif    
	return 0;
}
