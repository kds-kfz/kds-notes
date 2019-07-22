#ifndef JSON_H__
#define JSON_H__

#define JSON_NULL  0
#define JSON_True  1
#define JSON_False  2
#define JSON_Number  3
#define JSON_String  4
#define JSON_Array  5
#define JSON_Object  6
typedef struct json{
	struct json* next;
	struct json* child;//存数组或者对象
	char* str; //字符串
	double num;//数字
	char* key;//关键字
	int type;//类型
}JSON;
JSON* Parse(const char* p);
void Print(JSON* json);
void Delete(JSON* json);
#endif

