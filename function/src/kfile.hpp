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

using namespace std;

namespace kfz{

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
    bool Open(const char *filename, int flags){
        //因为类内有自己的文件描述符和缓存区，故要求在打开新文件之前必须关闭上一文件描述符、清空缓存
        if(this->_fd){
            if(_size != 1){
                close(this->_fd);
                delete []this->_data;
                this->_size = 1;
                this->_data = new char[1]{0};
            }else{
                close(this->_fd);
            }
        }

        if(flags == O_RDONLY || flags == O_WRONLY || flags == O_RDWR){
            if((_fd = open(filename, flags)) == -1){
                _fd = open(filename, O_CREAT|flags, 0666);
            }
        }else if(flags == O_APPEND){
            _fd = open(filename, O_CREAT|O_RDWR|flags, 0640);
        }
        /*
        if((_fd = open(filename, flags)) == -1){
            fprintf(stderr,"open file fail\n");
            return false;
        }else{*/
            if(flags == O_RDONLY || flags == O_RDWR){
                _size = lseek(_fd, 0L, SEEK_END);
                lseek(_fd, 0L, SEEK_SET);
                _data = new char[_size + 1];
                memset(_data, 0, _size + 1);
                //获取文件内容
                int ret;
                char ch;
                char *pbuf = _data;
                int byte = _size > 1024 ? 1024 : _size;
                while(1){
                    #if 0
                    //每次读取一个字节
                    if((ret = read(_fd, &ch, 1)) == -1){
                        fprintf(stderr,"read read file by one byte Error:[%s]\n",strerror(ret));
                        return false;
                    }else if(ret != 0){
                        memcpy(pbuf, &ch, ret);
                        pbuf += ret;
                    }else{
                        break;
                    }
                    #endif
                    //每次读取指定个数字节
                    char buf[1025] = {0};
                    if((ret = read(_fd, buf, byte)) == -1){
                        fprintf(stderr,"read read file by 1024 byte Error:[%s]\n",strerror(errno));
                        return false;
                    }else if(ret != 0){
                        memcpy(pbuf, buf, ret);
                        pbuf += ret;
                    }else{
                        break;
                    }
                }
           }
    //    }
        return true;
    }
    //生成一个新文件
    //先调用 Open 函数，在调用 Read 和 Write 函数
    bool Write(const char *basePath, const char *newfile, const char *content = NULL){
        //查找当前目录是否已经有该文件
        //cout<<strlen(content)<<endl;
        DIR *dir;
        struct dirent *ptr;
        bool flag = false;
        char olddir[512] = {0};
        char newdir[512] = {0};
        if(strcmp(basePath, "./") == 0){
            strcat(newdir, "./");
            strcat(newdir, newfile);
        }
        
        //拓展功能，支持指定目录生成文件，TODO：待兼容
        //绝对路径
        //getcwd(currdir, sizeof(newdir));
        //strcat(newdir, "/");
        //strcat(newdir, newfile);
        
        if((dir = opendir(basePath)) == NULL){
            perror("Open dir Error ...");
            return false;
        }
        while((ptr = readdir(dir)) != NULL){
            //跳过 . .. 以及隐藏文件或目录
            if(strncmp(ptr->d_name, ".", 1) == 0 || strcmp(ptr->d_name, "..") == 0){
                continue;
            }else if(ptr->d_type == 10){//link file
            
            }else if(ptr->d_type == 4){//dir
            
            }else if(ptr->d_type == 8){//file
                memset(olddir, '\0', sizeof(olddir));
                strcpy(olddir, basePath);
                strcat(olddir, ptr->d_name);
                if(strcmp(olddir, newdir) == 0){
                    flag = true;
                    break;
                }
            }
        }
        
        int fw = 0;
        int ret = 0;
        
        if(!flag){//文件还没有，就创建新文件
            if((fw = open(newdir ,O_CREAT|O_RDWR,0666)) == -1){
                cout<<"open newdir fail"<<endl;
                return false;
            }else{
                //默认写入读打开的文件内容，否则写入其他内容
                if(content == NULL && this->_data){
                    if((ret = write(fw, this->_data, this->_size)) == -1){
                        fprintf(stderr,"write to newfile Error[%s]", strerror(errno));
                        return false;
                    }
                }else{
                    if((ret = write(fw, content, strlen(content))) == -1){
                        fprintf(stderr,"write to newfile Error[%s]", strerror(errno));
                        return false;
                    }
                }
            }
            close(fw);
        }else{//文件已经存在
            Files ft;//局部类，离开作用于自动析构
            if(ft.Open(newdir, O_RDWR) && ft.Size() == 0){
                if(content == NULL && this->_data){
                    if((ret = write(ft.Handle(), this->_data, this->_size)) == -1){
                        fprintf(stderr,"write to newfile Error[%s]", strerror(errno));
                        return false;
                    }
                }else{
                    if((ret = write(ft.Handle(), content, strlen(content))) == -1){
                        fprintf(stderr,"write to newfile Error[%s]", strerror(errno));
                        return false;
                    }
                }
            }else{
                fprintf(stderr, "newfile is already exists and newfile the size is not eq 0\n");
                return false;
            }
        }
    }
    
    //单一功能写入指定内容
    bool Write(const char *content){
        if(this->_fd == -1){
            fprintf(stderr, "this write-file is non-existent!\n");
            return false;
        }
        //获取写入内容大小，考虑是否分批写入
        int ret = 0;
        char buff[1024] = {0};
        const char *pbuff = content;
        unsigned long size = strlen(content);
        unsigned long nbyte = size > sizeof(buff) ? sizeof(buff) : size;
        while(nbyte){
            memcpy(buff, pbuff, nbyte);
            if((ret = write(this->_fd, buff, strlen(buff))) == -1){
                fprintf(stderr,"write to newfile Error[%s]", strerror(errno));
                return false;
            }
            memset(buff, 0, sizeof(buff));
            pbuff += ret;
            nbyte = size - ret;
        }
        return true;
    }
    int Handle(){
        return _fd;
    }
    long Size(){
        return _size;
    }
    char *Date(){
        return _data;
    }
    //禁止调用该方法，否则会出现意外的段错误
    //只能在析构中调用
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
                cout<<"正在析构all,fd="<<this->_fd<<",data="<<this->_data<<endl;
                Close();
            }else{
                cout<<"正在析构,fd="<<this->_fd<<",data="<<this->_data<<endl;
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

}

#endif
