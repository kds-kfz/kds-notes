#ifndef __KMEMFILE_H__
#define __KMEMFILE_H__

#include <malloc.h>
#include <assert.h>

//TODO 待优化，使用 c++ 语言风格，封装成类似 string 自动递增的内存管理方式

class KMemFile{
protected:
    unsigned int  m_nGrowBytes;
    unsigned long m_nPosition;
    unsigned long m_nBufferSize;
    unsigned long m_nFileSize;
    unsigned char *m_lpBuffer;
    bool m_bAutoDelete;

    unsigned char *Alloc(unsigned long nBytes){
        return (unsigned char *)malloc((unsigned int)nBytes);
    }

    unsigned char *Realloc(unsigned char *lpMen, unsigned long nBytes){
        return (unsigned char *)realloc(lpMen, (unsigned int)nBytes);
    }

#pragma intrinsic(memcpy){
    unsigned char *Memcpy(unsigned char *lpMenTarget, const unsigned char *lpMenSource, unsigned int nBytes){
        assert(lpMenTarget != NULL);
        assert(lpMenSource != NULL);
    return (unsigned char *)memcpy(lpMenTarget, lpMenSource, nBytes);
    }
#pragma function(memcpy)

     void Free(unsigned char *lpMen){
        assert(lpMen != NULL);
        free(lpMen);
     }

     void GrowFile(unsigned long dwNewLen){
        assert(this);

        if(dwNewLen > m_nBufferSize){
            // grow the buffer
            unsigned long dwNewBufferSize = (unsigned long)m_nBufferSize;

            // watch out for buffers which cannot be grown
            assert(m_nGrowBytes != 0);

            // determine new buffer size
            while(dwNewBufferSize < dwNewLen)
                dwNewBufferSize += m_nGrowBytes;

            // allocate new buffer
            unsigned char *lpNew;
            if(m_lpBuffer == NULL){
                lpNew = Alloc(dwNewBufferSize);
            }else{
                lpNew = Realloc(m_lpBuffer, dwNewBufferSize);
            }

            m_lpBuffer = lpNew;
            m_nBufferSize = dwNewBufferSize;
        }

        assert(this);
     }

public:
     enum SeekPosition{ begin = 0x0, current = 0x1, end = 0x2 };

     // NGrowBytes 读取文件需要增大时的增大粒度 (每次增大 nGrowBytes 个字节)
     // 有参构造
     KMemFile(unsigned int nGrowBytes = 1024){
        m_nGrowBytes = nGrowBytes;
        m_nPosition = 0;
        m_nBufferSize = 0;
        m_nFileSize = 0;
        m_lpBuffer = NULL;
        m_bAutoDelete = true;
     }

     // 相对于构造后调用 Attach
     // 有参构造
     KMemFile(unsigned char *lpBuffer, unsigned int nBufferSize, unsigned int nGrowBytes = 0){
        m_nGrowBytes = nGrowBytes;
        m_nPosition = 0;
        m_nBufferSize = nBufferSize;
        m_nFileSize = nGrowBytes == 0 ? nBufferSize : 0;
        m_lpBuffer = lpBuffer;
        m_bAutoDelete = false;
     }

     // 析构
     ~KMemFile(){
        // Close should have already been called, but we check anyway
        if(m_lpBuffer){
            Close();
        }
        assert(m_lpBuffer == NULL);

        m_nGrowBytes = 0;
        m_nPosition = 0;
        m_nBufferSize = 0;
        m_nFileSize = 0;
     }

     // 取属性
     unsigned long GetPosition() const{
        assert(this);
        return m_nPosition;
     }

     // 取得文件的当前大小
     unsigned long GetLength() const{
        unsigned long dwLen, dwCur;
        // Seek is a non const operation
        KMemFile *pFile = (KMemFile *)this;
        dwCur = pFile->Seek(0L, current);
        dwLen = pFile->SeekToEnd();
        pFile->Seek(dwCur, begin);

        return dwLen;
     }

     // 取得内存数据指针
     // 注意：制度，不要修改该指针指向的内容，如果需要读写该文件内容，请用 Read() 和 Write()
     const unsigned char *const GetPtr() const{
        return m_lpBuffer;
     }

    // 操作
    // 设置自动增长尺寸
    void SetGrowBytes(unsigned int nGrowBytes = 1024){
        //assert(nGrowBytes <= unsigned int_MAX);

        m_nGrowBytes = nGrowBytes;
    }

    // 指定本文件所有的内存
    // 该对象那个析构时不会释放该内存块
    void Attach(unsigned char *lpBuffer, unsigned int nBufferSize, unsigned int nGrowBytes = 0){
        assert(m_lpBuffer == NULL);

        m_nGrowBytes = nGrowBytes;
        m_nPosition = 0;
        m_nBufferSize = nBufferSize;
        m_nFileSize = nGrowBytes == 0 ? nBufferSize : 0;
        m_lpBuffer = lpBuffer;
        m_bAutoDelete = false;
    }

    // 接触该文件和他当前占用的内存块的关联
    // 返回该内存块的指针
    unsigned char *Detach(){
        unsigned char *lpBuffer = m_lpBuffer;
        m_lpBuffer = NULL;
        m_nFileSize= 0;
        m_nBufferSize = 0;
        m_nPosition = 0;

        return lpBuffer;
    }

    // 移动读写文职到文件最后
    unsigned long SeekToEnd(){
        return Seek(0, KMemFile::end);
    }

    // 移动读写位置到文件开头
    void SeekToBegin(){
        Seek(0, KMemFile::begin);
    }

    // 改变文件长度
    // 如果需要会重新分配做占用内存
    void SetLength(unsigned long dwNewLen){
        assert(this);
        if(dwNewLen > m_nBufferSize){
            GrowFile(dwNewLen);
        }

        if(dwNewLen < m_nPosition){
            m_nPosition = dwNewLen;
        }

        assert(this);
    }

    // 从当前读写位置读取指定长度的数据
    unsigned int Read(const void *lpBuf, unsigned int nCount){
        assert(this);

        if(nCount == 0){
            return 0;
        }

        assert(lpBuf != NULL);
        //assert(AfxIsValidAddress(lpBuf, nCount));

        if(m_nPosition > m_nFileSize){
            return 0;
        }

        unsigned int nRead;
        if(m_nPosition + nCount > m_nFileSize){
            nRead = (unsigned int)(m_nFileSize - m_nPosition);
        }else{
            nRead = nCount;
        }

        Memcpy((unsigned char *)lpBuf, (unsigned char *)m_lpBuffer + m_nPosition, nRead);
        m_nPosition += nRead;

        assert(this);

        return nRead;
    }

    // 模板函数
    template <typename T>
    unsigned int Read(T &buf){
        return Read(&buf, sizeof(buf));
    }

    // 写指定长度的数据到文件内
    void Write(const void *lpBuf, unsigned int nCount, unsigned int nDataLen, bool bRightAlign = true, unsigned char ch = ' '){
        assert(this);

        if(nCount == 0){
            return ;
        }

        assert(lpBuf != NULL);
        //assert(AfxIsValidAddress(lpBuf, nCount, false));

        if(m_nPosition + nCount > m_nBufferSize){
            GrowFile(m_nPosition + nCount);
        }

        assert(m_nPosition + nCount <= m_nBufferSize);

        memset((unsigned char *)m_lpBuffer + m_nPosition, ch, nCount);
        if(bRightAlign && (nDataLen < nCount)){
            //memset((unsigned char *)m_lpBuffer + m_nPosition, '', nDataLen - nCount);
            Memcpy((unsigned char *)m_lpBuffer + m_nPosition - nDataLen,
                    (unsigned char *)lpBuf, nDataLen);
        }else{
            Memcpy((unsigned char *)m_lpBuffer + m_nPosition,
                    (unsigned char *)lpBuf, nDataLen < nCount ? nDataLen : nCount);
        }
        
        m_nPosition += nCount;

        if(m_nPosition > m_nFileSize){
            m_nFileSize = m_nPosition;
        }

        assert(this);
    }

    // 模板函数
    template <typename T>
    void Write(const T &buf){
        Write(&buf, sizeof(buf));
    }

    // 移动读写位置
    // loff 移动的距离
    // nFrom 指定从哪里开始移动
    // 第二个参数可以为
    // enum SeekPosition { begin = 0x0, current = 0x1, end = 0x2 };
    long Seek(long loff, unsigned int nFrom){
        assert(this);
        assert(nFrom == begin || nFrom == end ||nFrom == current);

        long lNewPos = m_nPosition;

        if(nFrom == begin){
            lNewPos = loff;
        }else if(nFrom == current){
            lNewPos += loff;
        }else if(nFrom == end){
            lNewPos = m_nFileSize + loff;
        }else{
            return -1;
        }

        if(lNewPos < 0){
            return -1;
            //throe exception("Seek 错误");
        }

        m_nPosition = lNewPos;

        assert(this);

        return m_nPosition;
    }

    // 关闭内存文件，释放锁占用的内存，置长度为0
    // 该文件不可增长 (即不可再写入任何数据)
    // 析构函数会自动调用 Colse()
    void Close(){
        assert((m_lpBuffer == NULL && m_nBufferSize == 0) ||
                !m_bAutoDelete || true);

        assert(m_nFileSize < m_nBufferSize);

        m_nGrowBytes = 0;
        m_nPosition = 0;
        m_nBufferSize = 0;
        m_nFileSize = 0;
        if(m_lpBuffer && m_bAutoDelete){
            Free(m_lpBuffer);
        }
        m_lpBuffer = NULL;
    }

};

#endif //__KMENFILE_H__
