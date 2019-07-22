#include <stdio.h>
#include "cJSON.h"
int main()
{
    const char* p = "{\"name\":\"zhangsan\"}";
     cJSON*  json = cJSON_Parse(p);
     printf ("%d\n",json->type);
     printf("%s\n",json->child->string);
     printf("%s\n",json->child->valuestring);

     const char* p1 = "12344";
     json = cJSON_Parse(p1);
     printf("%d\n",json->type);
     printf("%d\n",json->valueint);

     const char* p2 = "\"hello\"";
     json = cJSON_CreateString(p2);
     printf("%d\n",json->type);
     printf("%s\n",json->valuestring);

     const char* p3 = "[1,2,\"hello\",5]";
     json = cJSON_Parse(p3);
     printf("%d\n",json->type);
     printf("%d\n",json->child->valueint);
     printf("%d\n",json->child->next->valueint);
     printf("%s\n",json->child->next->next->valuestring);

    // [ 1,2,4,"hello",5,7.8,null,false,true]
}
