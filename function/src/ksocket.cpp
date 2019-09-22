#include"ksocket.hpp"

int Socket::SocketServeRead(){
    //设置通信时间
    //timeval tv;
    //tv.tv_sec = timeout / 1000;
    //tv.tv_usec = (timeout $ 1000) * 1000;
    //select(sfd->fd +1, sfd->buff, 0, 0, &tv);
    
    memset(buff, 0, sizeof(buff));
    //读取的字节数大于 240*384 = 92160 = 90 * 1024, 90k
    //此时需要循环读取
    //而使用 recv 读取
    #if 0
    int ret = read(cfd->fd, &(buff), sizeof(Msg_buff));
    if(ret == 0){
        errmsg = SocketShowAccept(false);
        //errmsg = "SocketServeRead Fail, client is breaked!";
        return 0;
    }else if(ret < 0){
        if(errno == EINTR){
            errmsg = "SocketServeRead Read EINTR!";
            return -1;
        }else{
            errmsg = "SocketServeRead Read Error!";
            return -2;
        }
    }
    return 1;
    #endif
    int count = 0;
    //有可能接收的请求字节数超长，故先检验包体长度是否合法
    if((count = read(cfd->fd, buff, 8)) != 8){
        WARN_TLOG("Recv Header Error, break!\n");
        return -1;
    }
    //这里要校验前8字节是否都为纯数字
    unsigned long pkg_len = atol(buff);
    INFO_TLOG("收到请求包体长度:[%d]\n", pkg_len);
    if(pkg_len <= 0 || pkg_len > REQ_MAX_LEN){
        ERROR_TLOG("The Length oF Reqmsg Is Illigle Max[%d]\n", REQ_MAX_LEN);
        return -2;
    }
    memset(buff, 0, sizeof(buff));
    int nbyte = 3;
    unsigned int tpkg_len = pkg_len;
    
    #if 0
    //每次最大读取 1024 字节
    char *pbuf = buff;
    while(nbyte){
        char TempBuff[1024] = {0};
        count = read(cfd->fd, TempBuff, nbyte);
        if(count < 0){
            if(errno == EINTR){
                count = 0;
                errmsg = "SocketServeRead Read EINTR!";
                return -3;
            }else{
                char temp[12] = {0};
                sprintf(temp, "%d", count);
                errmsg = "Recv Type is None Or Error:[" + string(strerror(errno)) + "][" + string(temp) + "]!";
                return -4;
            }
        }else if(count == 0){
            errmsg = SocketShowAccept(false);                
            return 1;
            //break;
        }/*
        else{
            errmsg = "SocketServeRead Read Error!";
            return -5;
        }*/
        if(nbyte == 3 && TempBuff[2] == '#'){
            strcpy(this->type, TempBuff);
            nbyte = tpkg_len > sizeof(TempBuff) ? sizeof(TempBuff) : tpkg_len;
        }else{
            tpkg_len -= count;
            nbyte = tpkg_len > sizeof(TempBuff) ? sizeof(TempBuff) : tpkg_len;
            memcpy(pbuf, TempBuff, count);
            pbuf += count;
        }
    }
    #else
    //本次优化；每次最大接收 1024 字节，然后拷贝到内存里，当接收消息完毕，就拷贝到 buff
    KMemFile reads;
    while(nbyte){
        char TempBuff[1024] = {0};
        count = read(cfd->fd, TempBuff, nbyte);
        if(count < 0){
            if(errno == EINTR){
                count = 0;
                errmsg = "SocketServeRead Read EINTR!";
                return -3;
            }else{
                char temp[12] = {0};
                sprintf(temp, "%d", count);
                errmsg = "Recv Type is None Or Error:[" + string(strerror(errno)) + "][" + string(temp) + "]!";
                return -4;
            }
        }else if(count == 0){
            errmsg = SocketShowAccept(false);                
            return 1;
            //break;
        }/*
        else{
            errmsg = "SocketServeRead Read Error!";
            return -5;
        }*/
        if(nbyte == 3 && TempBuff[2] == '#'){
            strcpy(this->type, TempBuff);
            nbyte = tpkg_len > sizeof(TempBuff) ? sizeof(TempBuff) : tpkg_len;
        }else{
            tpkg_len -= count;
            nbyte = tpkg_len > sizeof(TempBuff) ? sizeof(TempBuff) : tpkg_len;
            reads.Write(TempBuff, count, count);
        }
    }
    reads.Write("\0", 1, 1);
    reads.SeekToBegin();
    memcpy(buff, reads.GetPtr(), reads.GetLength());
    reads.Close();
    #endif
    return 1;
}

int Socket::SocketClientRead(){
    /*
    memset(&(buff), 0, sizeof(Msg_buff));
    int ret = read(cfd->fd, &(buff), sizeof(Msg_buff));
    //ret == -1
    if(ret == -1){
        errmsg = "SocketClientRead Error:[" + string(strerror(errno)) + "]!";
        return false;
    }else if(ret == 0){
        errmsg = "SocketClientRead the EOF or Read 0 Byte!";
        return false;
    }
    return true;
    */
    int count = 0;
    memset(buff, 0, sizeof(buff));
    
    //有可能接收的请求字节数超长，故先检验包体长度是否合法
    if((count = read(cfd->fd, buff, 8)) != 8){
        ERROR_TLOG("Recv Header Error, break!\n");
        return -1;
    }
    //这里要校验前8字节是否都为纯数字
    unsigned long pkg_len = atol(buff);
    if(pkg_len <= 0 || pkg_len > REQ_MAX_LEN){
        ERROR_TLOG("The Length oF Reqmsg Is Illigle Max[%d]\n", REQ_MAX_LEN);
        return -2;
    }
    
    memset(this->type, 0, sizeof(this->type));
    memset(buff, 0, sizeof(buff));
    int nbyte = 3;
    //每次最大读取 1024 字节
    //char *pbuf = buff;
    unsigned int tpkg_len = pkg_len;
    TEST_TLOG("收到应答包体长度:[%d]\n", pkg_len);
    //本次优化；每次最大接收 1024 字节，然后拷贝到内存里，当接收消息完毕，就拷贝到 buff
    KMemFile reads;
    #if 0
    while(nbyte){
        char TempBuff[1024] = {0};
        count = read(cfd->fd, TempBuff, nbyte);
        if(count < 0){
            if(errno == EINTR){
                count = 0;
                errmsg = "SocketServeRead Read EINTR!";
                return -3;
            }else{
                char temp[12] = {0};
                sprintf(temp, "%d", count);
                errmsg = "Recv Type is None Or Error:[" + string(strerror(errno)) + "][" + string(temp) + "]!";
                return -4;
            }
        }else if(count == 0){
            errmsg = "SocketClientRead the EOF or Read 0 Byte!";
            //return 1;
            break;
        }/*
        else if(count == -1){
            errmsg = "SocketClientRead Error:[" + string(strerror(errno)) + "]!";
        }*/
        
        if(nbyte == 3 && TempBuff[2] == '#'){
            strcpy(this->type, TempBuff);
            //第一次读取消息内容，默认读取整个包长
            //如果包长大于缓存大小，则每次最多接收缓存大小这么长的字节
            //及时实际包长很长，没此最大获取 sizeof(TempBuff) 个字节，直到获取完毕
            nbyte = tpkg_len > sizeof(TempBuff) ? sizeof(TempBuff) : tpkg_len;
        }else{
            tpkg_len -= count;
            nbyte = tpkg_len > sizeof(TempBuff) ? sizeof(TempBuff) : tpkg_len;
            memcpy(pbuf, TempBuff, count);
            pbuf += count;
        }
    }
    #else
    while(nbyte){
        char TempBuff[1024] = {0};
        count = read(cfd->fd, TempBuff, nbyte);
        if(count < 0){
            if(errno == EINTR){
                count = 0;
                errmsg = "SocketServeRead Read EINTR!";
                return -3;
            }else{
                char temp[12] = {0};
                sprintf(temp, "%d", count);
                errmsg = "Recv Type is None Or Error:[" + string(strerror(errno)) + "][" + string(temp) + "]!";
                return -4;
            }
        }else if(count == 0){
            errmsg = "SocketClientRead the EOF or Read 0 Byte!";
            //return 1;
            break;
        }/*
        else if(count == -1){
            errmsg = "SocketClientRead Error:[" + string(strerror(errno)) + "]!";
        }*/
        
        if(nbyte == 3 && TempBuff[2] == '#'){
            strcpy(this->type, TempBuff);
            //第一次读取消息内容，默认读取整个包长
            //如果包长大于缓存大小，则每次最多接收缓存大小这么长的字节
            //及时实际包长很长，没此最大获取 sizeof(TempBuff) 个字节，直到获取完毕
            nbyte = tpkg_len > sizeof(TempBuff) ? sizeof(TempBuff) : tpkg_len;
        }else{
            tpkg_len -= count;
            nbyte = tpkg_len > sizeof(TempBuff) ? sizeof(TempBuff) : tpkg_len;
            reads.Write(TempBuff, count, count);
        }
    }
    reads.Write("\0", 1, 1);
    reads.SeekToBegin();
    memcpy(buff, reads.GetPtr(), reads.GetLength());
    reads.Close();
    #endif
    return 1;

}


