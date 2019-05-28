#include <stdio.h>
#include <string.h>
#include "cJSON.h"
int main()
{
    /*
       字符串　　valuestring
       数字　　valueint
       null   type
       false
       true
       child
     */
    //数组元素类型不同
    cJSON* arr = cJSON_CreateArray();
    printf("%d\n",arr->type);
    cJSON* item1 = cJSON_CreateNull();
    printf("item1->type:%d\n",item1->type);
    printf("item1:%p\n",item1);

    cJSON_AddItemToArray(arr,item1);
    printf("arr->child:%p\n",arr->child);

    cJSON* item2 = cJSON_CreateString("hello");
    printf("item2->type :%d\n",item2->type);
    printf("item2->valuestring :%s\n",item2->valuestring);
    printf("item2:%p\n",item2);

    cJSON_AddItemToArray(arr,item2);
    printf("item1->next:%p\n",item1->next);

    cJSON* item3 = cJSON_CreateNumber(123);
    printf("item3->type : %d\n",item3->type);
    printf("item3->valueint: %d\n",item3->valueint);
    printf("item3:%p\n",item3);

    cJSON_AddItemToArray(arr,item3);
    printf("item2->next:%p\n",item2->next);

    cJSON* arr1 = cJSON_CreateArray();
    cJSON_AddItemToArray(arr1,cJSON_CreateString("hello"));
    cJSON_AddItemToArray(arr1,cJSON_CreateNull());

    cJSON_AddItemToArray(arr1,cJSON_CreateNumber(34.6));
    cJSON_AddItemToArray(arr,arr1);

    cJSON_AddItemToArray(arr,cJSON_CreateFalse());

    //    [null,"hello",123,["hello",null,34.6],false]
    char* p = cJSON_Print(arr);
    printf("p = %s\n",p);
    //类型相同的数组
    int a[5] = {1,2,3,4,5};
    float b[3] = {1.3,4.5,6.7};
    const  char* c[4] = {"哈哈","bbb","ccc","ddd"};

    cJSON* a1 = cJSON_CreateIntArray(a,5);
    cJSON* a2 = cJSON_CreateFloatArray(b,3);
    cJSON* a3 = cJSON_CreateStringArray(c,4);

    cJSON* a4 = cJSON_CreateArray();
    cJSON_AddItemToArray(a4,a1);
    cJSON_AddItemToArray(a4,a2);
    cJSON_AddItemToArray(a4,a3);

    p = cJSON_Print(a4);
    printf("%s\n",p);
    FILE* fp = fopen("3.json","wb");
    fwrite(p,strlen(p),1,fp);
    fclose(fp);
    cJSON_Delete(a4);
}
