#ifndef cJSON__h
#define cJSON__h
#ifdef __cplusplus
extern "C"
{
#endif

#define cJSON_False 0
#define cJSON_True 1
#define cJSON_NULL 2
#define cJSON_Number 3
#define cJSON_String 4
#define cJSON_Array 5
#define cJSON_Object 6
	
#define cJSON_IsReference 256

typedef struct cJSON {
	struct cJSON *next,*prev;  //
	struct cJSON *child;//存数组或者对象
	int type;//类型
	char *valuestring; //字符串
	int valueint;   //数字
	double valuedouble;
	char *string; //对象关键字
} cJSON;

typedef struct cJSON_Hooks {
      void *(*malloc_fn)(size_t sz);
      void (*free_fn)(void *ptr);
} cJSON_Hooks;

extern void cJSON_InitHooks(cJSON_Hooks* hooks);
extern cJSON *cJSON_Parse(const char *value);//获取JSON对象,解析JSON
extern char *cJSON_Print(cJSON *item);//根据对象指针获取JSON字符串形式数据
extern char *cJSON_PrintUnformatted(cJSON *item);
extern void cJSON_Delete(cJSON *c);//最后释放对象
extern int	cJSON_GetArraySize(cJSON *array);//解析数组,获取数组大小
extern cJSON *cJSON_GetArrayItem(cJSON *array,int item);//获取数组元素
extern cJSON *cJSON_GetObjectItem(cJSON *object,const char *string);//获取指定名称的节点指针
extern const char *cJSON_GetErrorPtr(void);
extern cJSON *cJSON_CreateNull(void);
extern cJSON *cJSON_CreateTrue(void);
extern cJSON *cJSON_CreateFalse(void);
extern cJSON *cJSON_CreateBool(int b);
extern cJSON *cJSON_CreateNumber(double num);
extern cJSON *cJSON_CreateString(const char *string);
extern cJSON *cJSON_CreateArray(void);//创建数组对象
extern cJSON *cJSON_CreateObject(void);//创建JSON主对象

extern cJSON *cJSON_CreateIntArray(int *numbers,int count);
extern cJSON *cJSON_CreateFloatArray(float *numbers,int count);
extern cJSON *cJSON_CreateDoubleArray(double *numbers,int count);
extern cJSON *cJSON_CreateStringArray(const char **strings,int count);

extern void cJSON_AddItemToArray(cJSON *array, cJSON *item);//向数组中增加元素（元素类型不一定相同）
extern void	cJSON_AddItemToObject(cJSON *object,const char *string,cJSON *item);//向对象中增加对象（字符串、数组、数字）
extern void cJSON_AddItemReferenceToArray(cJSON *array, cJSON *item);
extern void	cJSON_AddItemReferenceToObject(cJSON *object,const char *string,cJSON *item);

extern cJSON *cJSON_DetachItemFromArray(cJSON *array,int which);
extern void   cJSON_DeleteItemFromArray(cJSON *array,int which);
extern cJSON *cJSON_DetachItemFromObject(cJSON *object,const char *string);
extern void   cJSON_DeleteItemFromObject(cJSON *object,const char *string);
	
extern void cJSON_ReplaceItemInArray(cJSON *array,int which,cJSON *newitem);
extern void cJSON_ReplaceItemInObject(cJSON *object,const char *string,cJSON *newitem);

extern cJSON *cJSON_Duplicate(cJSON *item,int recurse);
extern cJSON *cJSON_ParseWithOpts(const char *value,const char **return_parse_end,int require_null_terminated);



#define cJSON_AddNullToObject(object,name)		cJSON_AddItemToObject(object, name, cJSON_CreateNull())
#define cJSON_AddTrueToObject(object,name)		cJSON_AddItemToObject(object, name, cJSON_CreateTrue())
#define cJSON_AddFalseToObject(object,name)		cJSON_AddItemToObject(object, name, cJSON_CreateFalse())
#define cJSON_AddBoolToObject(object,name,b)	cJSON_AddItemToObject(object, name, cJSON_CreateBool(b))
#define cJSON_AddNumberToObject(object,name,n)	cJSON_AddItemToObject(object, name, cJSON_CreateNumber(n))
//向对象中增加（字符）节点
#define cJSON_AddStringToObject(object,name,s)	cJSON_AddItemToObject(object, name, cJSON_CreateString(s))


#define cJSON_SetIntValue(object,val)			((object)?(object)->valueint=(object)->valuedouble=(val):(val))

#ifdef __cplusplus
}
#endif
#endif

