#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "json.h"
static const char* skip_whitespace(const char* p)
{
	while(*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')
		p++;
	return p;
}
static const char* parse_null(JSON* json,const char* p)
{
	if(p[0]=='n'&&p[1]=='u'&&p[2]=='l'&&p[3]=='l')
	{
		json->type = JSON_NULL;
		return p+4;
	}
	else
	{
		printf("null格式错误\n");
		return p;
	}
}
static const char* parse_false(JSON* json,const char* p)
{
	if(p[0]=='f' && p[1]=='a'&&p[2]=='l'&&p[3]=='s'&&p[4]=='e')
	{
		json->type = JSON_False;
		return p+5;
	}
	else
	{
		printf("false格式错误\n");
		return p;
	}
}
static const char*  parse_true(JSON* json,const char* p)
{
	if(p[0]=='t' && p[1]=='r'&&p[2]=='u'&&p[3]=='e')
	{
		json->type = JSON_True;
		return  p+4;
	}
	else
	{
		printf("true格式错误\n");
		return p;
	}

}
static const char* parse_string(JSON* json,const char* p)
{
	json->type = JSON_String;
	int count = 1;
	const char* s = p;
	s++;
	while(1)
	{
		char c = *s++;
		switch(c)
		{
			case '\"':
				count++;
				json->str = (char*)malloc(count);
				memcpy(json->str,p,count);
				return s;
			case '\\':
				{
					count++;
					switch(*s++)
					{
						case 'b':count++;break;
						case 'n':count++;break;
						case 't':count++;break;
						case '0':count++;break;
						case 'r':count++;break;
						case 'f':count++;break;
						case 'v':count++;break;
						case '\\':count++;break;
						case '\"':count++;break;
						default :
								  printf("字符串格式不正确\n");
								  return p;
					}
					break;
				}
			case '\0':
				printf("字符串没有结尾　格式不正确\n");
				return p;
			default : count++;
		}
	}

}
//只识别十进制数据
//0后面只能跟.
//e或者E后面只能跟整数
static const char* parse_number(JSON*json,const char* p)
{
	const char* s = p;
	if(*s == '-' || (*s >= '0' && *s <= '9'))
	{
		if(*s == '-')
			s++;
		if(*s == '0')
		{
			s++;
			if(*s >= '0' && *s <= '9')
			{
				printf("0后面数字错误\n");
				return p;
			}
		}
		else
			for(s++;*s>='0'&&*s<='9';s++);

		if(*s == '.')
		{
			s++;
			if(*s < '0' || *s > '9')
			{
				printf(".后格式错误\n");
				return p;
			}
			else
				for(;*s >= '0' && *s <= '9';s++);
		}
		if(*s == 'e' || *s == 'E')
		{
			s++;
			if(*s == '+' || *s == '-')
				s++;
			if(*s < '0' || *s > '9')
			{
				printf("e后面格式错误\n");
				return p;
			}
			else
			{
				if(*s == '0')
				{
					s++;
					if(*s)
					{
						printf("e 0后面格式错误\n");
						return p;
					}
				}
				for(;*s >= '0' && *s <= '9' ;s++);
			}
		}
		json->type = JSON_Number;
		json->num = strtod(p,NULL);
		return s;
	}
	else
	{
		printf("数字json格式不正确\n");
		return p;
	}
}
static const char* parse_value(JSON* json,const char* p);
static const char* parse_array(JSON*json,const char* p)
{
	json->type = JSON_Array;
	const char*  s = p;
	if(*s == '[')
		s++;
	s = skip_whitespace(s);
	if(*s == ']')
	{
		printf("空数组\n");
		return s+1;
	}
	JSON* item = (JSON*)malloc(sizeof(JSON));
	if(item)
		memset(item,0,sizeof(item));
	s = parse_value(item,s);
	json->child = item;
	JSON* n = item;
	n->next = NULL;
	s = skip_whitespace(s);
	while(*s == ',')
	{
		s++;
		s = skip_whitespace(s);
		item = (JSON*)malloc(sizeof(JSON));
		if(item)
			memset(item,0,sizeof(item));
		s = parse_value(item,s);
		n->next = item;
		n = item;
		n->next = NULL;
		s = skip_whitespace(s);
	}
	if(*s == ']')
	{
		printf("数组解析成功\n");
		return s+1;
	}
	else
	{
		printf("数组格式错误\n");
		return p;
	}
}
static const char* parse_object(JSON* json,const char* p)
{
	json->type = JSON_Object;
	const char* s = p;
	if(*s == '{')
		s++;
	s = skip_whitespace(s);
	if(*s == '}')
	{
		printf("空对象\n");
		return s+1;
	}
	JSON* item = (JSON*)malloc(sizeof(JSON));
	if(item)
		memset(item,0,sizeof(JSON));
	JSON* n = NULL;
	s = parse_value(item,s);
	item->key = item->str;
	item->str = NULL;
	s = skip_whitespace(s);
	if(*s == ':')
	{
		s++;
		s = skip_whitespace(s);
		s = parse_value(item,s);
		json->child = item;
		item->next = NULL;
		n = item;

	}
	else
	{
		printf("对象格式不正确\n");
		return p;
	}
	s = skip_whitespace(s);
	while(*s == ',')
	{
		s++;
		s = skip_whitespace(s);
		item = (JSON*)malloc(sizeof(JSON));
		if(item)
			memset(item,0,sizeof(JSON));
		s = parse_value(item,s);
		item->key = item->str;
		item->str = NULL;
		s = skip_whitespace(s);
		if(*s == ':')
		{
			s++;
			s = skip_whitespace(s);
			s = parse_value(item,s);
			n->next = item;
			item->next = NULL;
			n = item;
			s = skip_whitespace(s);
		}
	}
	if(*s == '}')
	{
		printf("对象解析成功\n");
		return s+1;
	}
	else
	{
		printf("1对象格式不正确\n");
		return p;

	}
}

static const char* parse_value(JSON* json,const char* p)
{
	switch(*p)
	{
		case 'n' : p =  parse_null(json,p);break;
		case 'f' : p = parse_false(json,p);break;
		case 't' : p = parse_true(json,p);break;
		case '"' : p = parse_string(json,p);break;
		case '[' : p = parse_array(json,p);break;
		case '{' : p = parse_object(json,p);break;
		default  : p = parse_number(json,p);break; 
	}
	return p;
}
JSON* Parse(const char* p)
{
	const char* value = p;
	value = skip_whitespace(value);
	if(*value)
	{
		JSON* json = (JSON*)malloc(sizeof(JSON));
		if(json)
			memset(json,0,sizeof(JSON));
		value = parse_value(json,value);
		value = skip_whitespace(value);
		if (*value == '\0')
		{
			printf("解析成功\n");
			return json;
		}
		else
		{
			printf("格式不正确 解析失败\n");
			Delete(json);
			return NULL;
		}
	}
	else
	{
		printf("空\n");
		return NULL;
	}
}

//打印---------------------------------------------------------

void Print(JSON* json);
static void print_array(JSON* json1)
{
	JSON* json = json1->child;
	if (json)
	{
		printf ("[");
		while(json)
		{
			Print(json);
			json = json->next;
			if (json)
				printf(",");
		}
		printf ("]");
	}
	else
	{
		printf ("[]");
	}
}
static void print_object(JSON* json)
{
	JSON* json1 = json->child;
	if(json1)
	{
		printf("{\n\t");
		while(json1)
		{
			printf("%s : ",json1->key);
			Print(json1);
			json1 = json1->next;
			if(json1)
				printf(",\n\t");
			else
				printf("\n");
		}
		printf("}\n");
	}
	else
	{
		printf("{}");
	}
}
void Print(JSON* json)
{
	if(json)
	{

		if(json->type == JSON_String)
			printf("%s",json->str);
		else if(json->type == JSON_NULL)
			printf("null");
		else if (json->type == JSON_False)
			printf("false");
		else if (json->type == JSON_True)
			printf("true");
		else if (json->type == JSON_Number)
			printf("%.3f",json->num);
		else if (json->type == JSON_Array)
			print_array(json);
		else if(json->type == JSON_Object)
			print_object(json);
	}
}
static void delete(JSON* json)
{
	if(json)
	{
		if(json ->type == JSON_String)
			free(json->str);
		else if(json ->type == 5 || json->type == 6)
		{
			if(json->type == 6)
				free(json->key);
			JSON* item = json->child;
			while(item)
			{
				delete(item);
				item = item->next;
			}
		}
		else
			free(json);
	}
}
void Delete(JSON* json)
{
	if (json )
	{
		delete(json);
		free(json);
	}
}

