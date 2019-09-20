#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>

#include "cJSON.h"

using namespace std;

cJSON *OcrResInit(int num){
	cJSON* obj = cJSON_CreateObject();
    
    //在此对象中添加剩余对象
	cJSON_AddNumberToObject(obj,"all_ocr",0);
	cJSON_AddNumberToObject(obj,"dataDataLen",0);
	cJSON_AddNumberToObject(obj,"dataFormat",0);
    cJSON_AddNumberToObject(obj,"dataIndex",0);
	cJSON_AddNumberToObject(obj,"pattern_sn",0);
	cJSON_AddNumberToObject(obj,"processRst",0);
    cJSON_AddNumberToObject(obj,"regionNum",num);
    
    return obj;
}

cJSON *OcrResObj(const char *content, int type, int num){
	
    cJSON* obj_1 = cJSON_CreateObject();
    cJSON* arr_1 = cJSON_CreateArray();
	
	cJSON_AddStringToObject(obj_1,"content",content);
    cJSON_AddNumberToObject(obj_1,"height",38.0);
    cJSON_AddNumberToObject(obj_1,"isRecog",1);
    cJSON_AddNumberToObject(obj_1,"isVerified",0);
    cJSON_AddNumberToObject(obj_1,"lineType",1);
    cJSON_AddNumberToObject(obj_1,"line_no",0);
    cJSON_AddNumberToObject(obj_1,"sn_Inner",num);
    cJSON_AddNumberToObject(obj_1,"width",377);
    cJSON_AddNumberToObject(obj_1,"x",381.0);
    cJSON_AddNumberToObject(obj_1,"y",643.0);
    
    //向数组中添加对象
    cJSON_AddItemToArray(arr_1, obj_1);
    
    cJSON* obj_33 = cJSON_CreateObject();
	cJSON_AddNumberToObject(obj_33,"linNum",1);
    
    cJSON_AddItemToObject(obj_33, "lines", arr_1);
	cJSON_AddNumberToObject(obj_33,"regionType",type);

	//cJSON* arr_33 = cJSON_CreateArray();
    //数组中放入对象
    //以后的识别要素都这样添加到数组中
    //这后两句要放到外面写
    //cJSON_AddItemToArray(arr_33, obj_33);

    return obj_33;
}

cJSON *OcrResList(int num, cJSON *arr_33){
    cJSON *obj = OcrResInit(num);
    
    //对象中添加对象
    cJSON_AddItemToObject(obj, "regions", arr_33);
    //在此对象中添加剩余对象
	cJSON_AddNumberToObject(obj,"regionType",1);
    
    //向数组中添加对象
	cJSON* arr_2 = cJSON_CreateArray();
    cJSON_AddItemToArray(arr_2, obj);
    //向对象中插入对象
    cJSON_AddNumberToObject(obj,"dataNum",1);

    cJSON* obj_3 = cJSON_CreateObject();
    //向对象中插入对象
	cJSON_AddStringToObject(obj_3,"RstCode","0000");
    //对象中添加对象

    //cJSON_AddItemToObject(obj_3, "dataList", arr_2);
    //向对象中插入对象，值是空
    cJSON* obj_4 = cJSON_CreateObject();
    cJSON_AddItemToObject(obj_4, "dataList", arr_2);
    char *p = cJSON_Print(obj_4);
    string temp;
    temp = "[" + string(p) + "]";
    //cout<<temp<<endl;
    cJSON_AddStringToObject(obj_3,"RstMesg", temp.c_str());

	cJSON_AddNullToObject(obj_3,"IDFilewPath");
	cJSON_AddNullToObject(obj_3,"HeadFilePath");
	cJSON_AddNullToObject(obj_3,"filename");
    
    free(p);
    return obj_3;
}

cJSON *OcrResListNull(){
    cJSON *obj = OcrResInit(0);
    
    //对象中添加对象
    cJSON_AddNullToObject(obj, "regions");
    //在此对象中添加剩余对象
	//cJSON_AddNumberToObject(obj,"regionType",1);
    
    //向数组中添加对象
	cJSON* arr_2 = cJSON_CreateArray();
    cJSON_AddItemToArray(arr_2, obj);

    cJSON* obj_3 = cJSON_CreateObject();
    //向对象中插入对象
	cJSON_AddStringToObject(obj_3,"RstCode","0000");
    //对象中添加对象

    //创建 dataList 对象
    cJSON* obj_4 = cJSON_CreateObject();
    cJSON_AddItemToObject(obj_4, "dataList", arr_2);
    cJSON_AddNumberToObject(obj_4,"dataNum",1);
    char *p = cJSON_Print(obj_4);
    string temp;
    temp = "[" + string(p) + "]";
    //cout<<temp<<endl;
    cJSON_AddStringToObject(obj_3,"RstMesg", temp.c_str());

	cJSON_AddNullToObject(obj_3,"IDFilewPath");
	cJSON_AddNullToObject(obj_3,"HeadFilePath");
	cJSON_AddNullToObject(obj_3,"filename");
    
    free(p);
    return obj_3;
}

int main()
{
#if 1
    cJSON *obj_1 = OcrResObj("49016196512025076", 1, 3);
    cJSON *obj_2 = OcrResObj("第一代居民身份证", 2, 127);
    cJSON *obj_3 = OcrResObj("阿诺斯瓦辛格", 2, 2);
    cJSON *obj_4 = OcrResObj("56093197711267809", 1, 21);
    cJSON *obj_5 = OcrResObj("第二代身份证", 2, 13);
    cJSON *obj_6 = OcrResObj("电竞王力宏", 2, 10);
	
    //lines
    cJSON *arr_1 = cJSON_CreateArray();
    cJSON_AddItemToArray(arr_1, obj_1);
    cJSON_AddItemToArray(arr_1, obj_2);
    cJSON_AddItemToArray(arr_1, obj_3);
    
    cJSON *arr_2 = cJSON_CreateArray();
    cJSON_AddItemToArray(arr_2, obj_4);
    cJSON_AddItemToArray(arr_2, obj_5);
    cJSON_AddItemToArray(arr_2, obj_6);
    
    //删除，数组中第一个节点，释放内存
    //cJSON_DeleteItemFromArray(arr, 0);

    //list
    cJSON *obj1 = OcrResList(3, arr_1);
    
    char* p = cJSON_Print(obj1);
	//printf ("%s\n",p);
	FILE* fp = fopen("../data/date_ocr_res_2.json","wb");
	if(fp == NULL){
        cout<<"目录不存在"<<endl;
        exit(1);
    }
    fwrite(p,strlen(p),1,fp);
	fclose(fp);

    //这里释放外层的cJSON *指针即可
	cJSON_Delete(obj1);
    free(p);

    /************************************/
    //list
    cJSON *obj2 = OcrResList(3, arr_2);
    
    char* p2 = cJSON_Print(obj2);
	//printf ("%s\n",p);
	FILE* fp2 = fopen("../data/date_ocr_res_3.json","wb");
	if(fp2 == NULL){
        cout<<"目录不存在"<<endl;
        exit(1);
    }
    fwrite(p2, strlen(p2), 1, fp2);
	fclose(fp2);

    //这里释放外层的cJSON *指针即可
	cJSON_Delete(obj2);
    free(p2);

    /************************************/
    cJSON *obj3 = OcrResListNull();
    
    char* p3 = cJSON_Print(obj3);
	//printf ("%s\n",p);
	FILE* fp3 = fopen("../data/date_ocr_res_5.json","wb");
	if(fp2 == NULL){
        cout<<"目录不存在"<<endl;
        exit(1);
    }
    fwrite(p3, strlen(p3), 1, fp3);
	fclose(fp3);

    //这里释放外层的cJSON *指针即可
	cJSON_Delete(obj3);
    free(p3);
#endif
#if 0
    //原始实现方式
	cJSON* obj_1 = cJSON_CreateObject();
	cJSON* arr_1 = cJSON_CreateArray();
	
	cJSON_AddStringToObject(obj_1,"content","49016196512025076");
    cJSON_AddNumberToObject(obj_1,"height",38.0);
    cJSON_AddNumberToObject(obj_1,"isRecog",1);
    cJSON_AddNumberToObject(obj_1,"isVerified",0);
    cJSON_AddNumberToObject(obj_1,"lineType",1);
    cJSON_AddNumberToObject(obj_1,"line_no",0);
    cJSON_AddNumberToObject(obj_1,"sn_Inner",3);
    cJSON_AddNumberToObject(obj_1,"width",377);
    cJSON_AddNumberToObject(obj_1,"x",381.0);
    cJSON_AddNumberToObject(obj_1,"y",643.0);
    //向数组中添加对象
    cJSON_AddItemToArray(arr_1, obj_1);

    //cJSON* obj_22 = cJSON_CreateObject();
    //对象中添加对象
    //cJSON_AddItemToObject(obj_22, "lines", arr_1);

    cJSON* obj_33 = cJSON_CreateObject();
	cJSON_AddNumberToObject(obj_33,"linNum",1);
    
    cJSON_AddItemToObject(obj_33, "lines", arr_1);
	cJSON_AddNumberToObject(obj_33,"regionType",1);

    
	cJSON* arr_33 = cJSON_CreateArray();
    //数组中放入对象
    //以后的识别要素都这样添加到数组中
    cJSON_AddItemToArray(arr_33, obj_33);
    
    cJSON* obj_2 = cJSON_CreateObject();

    //在此对象中添加剩余对象
	cJSON_AddNumberToObject(obj_2,"all_ocr",0);
	cJSON_AddNumberToObject(obj_2,"dataDataLen",0);
	cJSON_AddNumberToObject(obj_2,"dataFormat",0);
	cJSON_AddNumberToObject(obj_2,"pattern_sn",0);
	cJSON_AddNumberToObject(obj_2,"processRst",0);
	cJSON_AddNumberToObject(obj_2,"regionNum",1);
    
    //对象中添加对象
    cJSON_AddItemToObject(obj_2, "regions", arr_33);
    //在此对象中添加剩余对象
	cJSON_AddNumberToObject(obj_2,"regionType",1);
    
    //向数组中添加对象
	cJSON* arr_2 = cJSON_CreateArray();
    cJSON_AddItemToArray(arr_2, obj_2);
    //向对象中插入对象
    cJSON_AddNumberToObject(obj_2,"dataNum",1);

    //插入回车
    //cJSON *item = cJSON_CreateString("\n");
    //cJSON_AddItemToArray(arr_2, item);

    cJSON* obj_3 = cJSON_CreateObject();
    //向对象中插入对象
	cJSON_AddStringToObject(obj_3,"RstCode","0000");
    //对象中添加对象
    cJSON_AddItemToObject(obj_3, "dataList", arr_2);
    //向对象中插入对象，值是空
	cJSON_AddNullToObject(obj_3,"IDFilewPath");
	cJSON_AddNullToObject(obj_3,"HeadFilePath");
	cJSON_AddNullToObject(obj_3,"filename");

	char* p = cJSON_Print(obj_3);
	printf ("%s\n",p);
	FILE* fp = fopen("4.json","wb");
	fwrite(p,strlen(p),1,fp);
	fclose(fp);

    //这里释放外层的cJSON *指针即可
	cJSON_Delete(obj_3);
    free(p);
#endif


	return 0;
}
