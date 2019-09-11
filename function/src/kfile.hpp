#ifndef __KFILE__H__
#define __KFILE__H__

#include <iostream>
#include <string>

#include <cstdio>
#include <cstring>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

//这里使用宏定义, 预编译, FILE_MODE: 0: 采用系统函数 1: 采用库函数
#define FILE_MODE  0

# ifndef LOG_MODULE
# define LOG_MODULE "FILE "
# endif


//文件处理类
class Files{
#if FILE_MODE == 0
private:
    int _fd;
    off_t _size;
    char *_data;
public:
    //无参构造函数
    Files(){
        _fd = 0;
        _size = 1;
        _data = new char[1];
        *_data = '\0';
    }
    
    /*打开文件 flags 取值
    O_RDONLY   只读
    O_WRONLY   只写 文件必须存在
    O_CREAT    创建（文件加权限）
    O_TRUNC    清空
    O_EXCL     判断文件是否存在
    O_APPEND   追加写
    O_RDWR     读写
    ---------------------------------------------
    stdin  STDIN_FIFONO    标准输入 0
    stdout STDOUT_FIFENO   标准输出 1
    stderr STDERR_FIFENO   标准错误 2
    */
    bool Open(const char *filename, int flags);
    
    //生成一个新文件，先调用 Open 函数，在调用 Read 和 Write 函数
    bool Write(const char *basePath, const char *newfile, const char *content = NULL);
    
    //单一功能写入指定内容
    bool Write(const char *content);

    int Handle(){
        return _fd;
    }
    
    long Size(){
        return _size;
    }
    
    char *Data(){
        return _data;
    }
    
    //禁止调用该方法，否则会出现意外的段错误，只能在析构中调用
    void Close(){
        close(this->_fd);
        delete []this->_data;
        this->_size = 0;
    }

    //析构函数
    ~Files(){
        //如果文件描述符非0，那么 _data 才有可能站有内存
        if(this->_fd){
            if(_data){
                Close();
            }else{
                close(this->_fd);
                this->_size = 0;
            }
        }
    }

#else
//采用 c 库函数
private:
	FILE* _fp;
    off_t _size;
    char *_data;
public:
    //无参构造函数
    Files(){
        _fp = 0;
        _size = 0;
        _data = (char *)malloc(1);
        *_data = '\0';
    }
    
    bool Open(const char *filename, int flags);
    
    bool Write(const char *basePath, const char *newfile, const char *content = NULL);
    
    ~Files(){}
#endif
};

#endif
