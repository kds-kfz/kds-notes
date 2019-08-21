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

using namespace std;

namespace kfz{

//文件处理类
class Files{
private:
    int _fd;
    off_t _size;
    char *_date;
public:
#if 0
    //打开文件
    bool open(const char *filename, const char *mode){
        _fp = fopen(filename, mode);
	    if(_fp == NULL){
            std::cout<<"open Error"<<std::endl;
	        return false;
	    }
        //计算文件大小
        _size = fseek(_fp, 0, SEEK_END) == 0 ? ftell(_fp) : -1;
        //重置文件光标位置
        fseek(_fp, 0, SEEK_SET);
        //获取文件内容
        _date = new char[_size + 1];
        //fscanf 读到\0 或者空格结束，但是空文件，此方法不可行
        char *pdate = _date;
	    while(1){
            char buf[1024] = {0};
            int ret = fscanf(_fp, "%s", buf);
            if(ret < 0){
                std::cout<<"fscanf fail"<<std::endl;
                return false;
            }else if(ret == 1){
                memcpy(pdate, buf, strlen(buf));
                pdate += strlen(buf);
            }else{
                break;
            }
        }
        //读到换行或文件末尾结束，此方法不行
	    //fgets(_date, _size, _fp);
        return true;
    }
#endif
    //无参构造函数
    Files(){
        _fd = 0;
        _size = 0;
        _date = new char[1];
        *_date = '\0';
    }
    //打开文件
    bool Open(const char *filename, int flags){
        if((_fd = open(filename, flags)) == -1){
            std::cout<<"open file fail"<<std::endl;
            return false;
        }else{
            _size = lseek(_fd, 0L, SEEK_END);
            lseek(_fd, 0L, SEEK_SET);
            _date = new char[_size + 1];
            memset(_date, 0, _size + 1);
            //获取文件内容
            int ret;
            char ch;
            char *pbuf = _date;
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
                    fprintf(stderr,"read read file by 1024 byte Error:[%s]\n",strerror(ret));
                    return false;
                }else if(ret != 0){
                    memcpy(pbuf, buf, ret);
                    pbuf += ret;
                }else{
                    break;
                }
            }
        }
        return true;
    }
    //生成一个新文件
    bool Write(const char *basePath, const char *newfile, const char *content = NULL){
        //查找当前目录是否已经有该文件
        cout<<strlen(content)<<endl;
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
        
        if(!flag){//文件还没有
            if((fw = open(newdir ,O_CREAT|O_RDWR,0666)) == -1){
                cout<<"open newdir fail"<<endl;
                return false;
            }else{
                if(content == NULL){
                    if((ret = write(fw, this->_date, this->_size)) == -1){
                        fprintf(stderr,"write to newfile Error[%s]", strerror(ret));
                        return false;
                    }
                }else{
                    if((ret = write(fw, content, strlen(content))) == -1){
                        fprintf(stderr,"write to newfile Error[%s]", strerror(ret));
                        return false;
                    }
                }
            }
            close(fw);
        }else{//文件已经存在
            Files ft;//局部类，离开作用于自动析构
            if(ft.Open(newdir, O_RDWR) && ft.Size() == 0){
                if(content == NULL){
                    if((ret = write(ft.Handle(), this->_date, this->_size)) == -1){
                        fprintf(stderr,"write to newfile Error[%s]", strerror(ret));
                        return false;
                    }
                }else{
                    if((ret = write(ft.Handle(), content, strlen(content))) == -1){
                        fprintf(stderr,"write to newfile Error[%s]", strerror(ret));
                        return false;
                    }
                }
            }else{
                fprintf(stderr, "newfile is already exists and newfile the size is not eq 0\n");
                return false;
            }
        }
    }
    int Handle(){
        return _fd;
    }
    long Size(){
        return _size;
    }
    char *Date(){
        return _date;
    }
    //析构函数
    ~Files(){
        if(_fd){
            close(_fd);
        }
        delete []_date;
        _size = 0;
    }
};

}

#endif
