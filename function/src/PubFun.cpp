#include <iostream>
#include <string.h>
#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <vector>
#include <stdarg.h>
#include <map>

using namespace std;

typedef struct task_node_t{
    char AppSysCode[128];
    char BranchCode[128];
}task_node_t;


//函数功能：删除字符串中出现空格前的字符，前提是字符串中末尾不能出现空格
//demo："hello world read" --> "read"
int CopyStringWithoutEmppty(char *dst, char *src, int len){
    int pos = 0, tmppos = 0;
    strncpy(dst, src, len);
    string tmpstr = string(dst);
    while(tmppos = tmpstr.find(' ') != -1 && pos <len){
        pos++;
        tmpstr = string(dst + pos);
    }
    memset(dst, 0, len);
    strncpy(dst, tmpstr.c_str(), tmpstr.size());
}

//函数功能：读取目录下指定格式的所有文件路径到vector容器
//demo：readFileList(".", ".md", files);
int readFileList(const char *basePath, const char *mark, vector<string> &files){
    DIR *dir;
    struct dirent *ptr;
    char base[1024] = {0};
    if((dir = opendir(basePath)) == NULL){
        perror("Open dir error ...");
        exit(1);
    }
    while((ptr = readdir(dir)) != NULL){
        //跳过 . .. 以及隐藏文件或目录
        if(strncmp(ptr->d_name, ".", 1) == 0 || strcmp(ptr->d_name, "..") == 0){
            continue;
        }else if(ptr->d_type == 8){//file
            if(strstr(ptr->d_name, mark) != 0){
                memset(base, '\0', sizeof(base));
                strcpy(base, basePath);
                strcat(base, "/");
                strcat(base, ptr->d_name);
                files.push_back(base);
            }
        }else if(ptr->d_type == 10){//link file
            cout<<"link file==>d_name:"<<basePath<<ptr->d_name<<endl;
        }
        else if(ptr->d_type == 4){//dir 是目录则递归调用
            memset(base, '\0', sizeof(base));
            strcpy(base, basePath);
            strcat(base, "/");
            strcat(base, ptr->d_name);
            readFileList(base, mark, files);
        }
    }
    closedir(dir);
    return 0;
}

//函数功能：以任何格式打印出数据
//demo：debugout("当前日期：%s", GetTime("%Y%m%d").c_str());
void debugout(const char *fmt, ...){
    char buf[512];
    try{
        va_list ap;
        va_start(ap, fmt);
        vsnprintf(buf, sizeof(buf) - 1, fmt, ap);
        va_end(ap);
        cout<<buf<<endl;
    }catch(...){
        cout<<"exception in construct debug message"<<endl;
    }
}

//函数功能：获取本地时间
//demo：%Y->年，%m->月，%d->日，%H->时，%M->分，%S->秒
static string GetTime(const string& fmt)
{
    time_t tmt = time(NULL);
    struct tm *pTime = localtime(&tmt);
    char   timeBuf[64]={0};
    strftime(timeBuf,sizeof(timeBuf),fmt.c_str(),pTime);
    return string(timeBuf);
}

//函数功能：从字符串中截取字段到vector容器，分隔符是个字符，暂不支持字符字符有效校验
//demo：StringSplit((char *)":12:::2:3:4:a::33::", ':', files)
int StringSplit(char *src, const char split, vector<string> &files){
    int start = 0;
    int end = 0;
    int nSpiltCount = 0;
    char szTmp[256] = {0};
    string szCurrent = "";

    int nLength = strlen(src);
    if(nLength == 0){
        return 0;
    }

    for(int i = 0; i < nLength; i++){
        if(split == src[i]){
            //判断第一个分隔符出现前的字符和中间分隔符是否合法
            if(i == 0 || i > 0 && split == src[i - 1]){
                start += 1;
                continue;
            }
            nSpiltCount++;
            end = i;
            memcpy(szTmp, src + start, end - start);
            files.push_back(string(szTmp));
            start = end + 1;
        }
        memset(szTmp, 0, sizeof(szTmp));
    }
    //判断分隔符到结尾是否是有效字符
    if(start != nLength){
        memcpy(szTmp, src + start, nLength - start);
        files.push_back(string(szTmp));
    }
    //TODO
    //字符获取完成，字符有效校验待处理
    //写一个函数可获取数字，字符，特殊符号的函数对其校验
    return nSpiltCount;
}

//函数功能：从string中截取字段到vector容器
//demo：
int stringSplit( const string str, const string sep, vector <string> &vec )
{
    string::size_type begin, end;

    if ( str.length() == 0 ){
        return 0;
    }

    begin = 0;
    while(1){
        //end = str.find_first_of( sep, begin );
        end = str.find( sep, begin );
        if( end == string::npos ){
            break;
        }
        vec.push_back( str.substr( begin, end-begin ) );
        begin = end + sep.length();
    }
    vec.push_back( str.substr(begin) );

    return vec.size();
}

int main(int argc, char *argv[]){
    task_node_t node;
    vector<string> files;
    files.clear();
    memset(&node, 0, sizeof(node));
    char tempstr[64] = " hello world read";
    CopyStringWithoutEmppty(node.AppSysCode, tempstr, sizeof(tempstr));
    cout<<"["<<node.AppSysCode<<"]"<<endl;

    //readFileList("/root/github/kds-notes", ".cpp", files);
    cout<<"有效字符个数："<<StringSplit((char *)":12:::& $2:3:4:a::33::", ':', files)<<endl;
    for(vector<string>::iterator it = files.begin(); it != files.end(); it++){
        cout<<*it<<endl;
    }
    
    debugout("当前日期：%s", GetTime("%Y%m%d%H%M%S").c_str());
    return 0;
}
