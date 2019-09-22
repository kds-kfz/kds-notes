#include"kfile.hpp"

using namespace std;

bool Files::Open(const char *filename, int flags){
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
        _fd = open(filename, O_RDWR|flags);
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
            //char ch;
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
    return true;
}

bool Files::Write(const char *basePath, const char *newfile, const char *content){
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
    return true;
}

bool Files::Write(const char *content){
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


