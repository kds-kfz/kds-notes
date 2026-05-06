#if !defined(__XSDK_DBF_H__)
#define __XSDK_DBF_H__

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "xsdk_define.h"


//------------------------------------------------------------------------------
// DBF数据文件参量定义
#define FIELD_NAME_LEN                          10
#define MAX_FIELD_LEN                           255

#define DBF_LOCK_OFFSET                         0x40000000L
#define LOCK_FILE_BYTES                         0x3FFFFFFFL

//------------------------------------------------------------------------------
#define XSDK_OPEN_FILE_ERROR                    0x0001
#define XSDK_SEEK_FILE_ERROR                    0x0002
#define XSDK_READ_FILE_ERROR                    0x0003
#define XSDK_WRITE_FILE_ERROR                   0x0004
#define XSDK_LOCK_FILE_ERROR                    0x0005
#define XSDK_UNLOCK_FILE_ERROR                  0x0006
#define XSDK_CHANGE_SIZE_ERROR                  0x0007

#define XSDK_FILE_NOT_OPEN                      0x0008
#define XSDK_OPEN_MODE_ERROR                    0x0009
#define XSDK_RECORD_NO_ERROR                    0x000A
#define XSDK_FILE_ID_ERROR                      0x000B

#define XSDK_MEMORY_ERROR                       0xFFFE

#if defined(OS_IS_LINUX)
  #define _LARGEFILE_SOURCE
  #define _LARGEFILE64_SOURCE
  #define _FILE_OFFSET_BITS 64
#endif

BGN_NAMESPACE_XSDK

//------------------------------------------------------------------------------
class CDbfField
{
public:
  CDbfField(void) {m_pszData = NULL;}
  CDbfField(
    const char *p_pszName, 
    char p_chType, 
    int p_iLength, 
    int p_iDecLen = 0, 
    int p_iOffset = 0, 
    const char *p_pszData = NULL, 
    int p_iDataLen = 0
  );
  ~CDbfField();

public:
  CDbfField & SetData(
    const char *p_pszData, 
    int p_iDataLen
  );

  CDbfField & operator =(char p_chData);
  CDbfField & operator =(char *p_pszData);
  CDbfField & operator =(const char *p_pszdata);
  CDbfField & operator =(int p_iData);
  CDbfField & operator =(long long p_llData);
  CDbfField & operator =(float p_fData);
  CDbfField & operator =(double p_dData);

public:
  void GetValue(
    char *p_pszValue, 
    long p_lSize
  ) 
  {
    memset((void *)p_pszValue, 0x00, p_lSize);
    memcpy((void *)p_pszValue, (void *)m_pszData, xsdk_min(p_lSize, m_iDataLen));
  }
  void GetValue(char &p_refchValue) {p_refchValue = m_pszData[0];}
  void GetValue(int &p_refiValue) {p_refiValue = atoi(m_pszData);}
  void GetValue(long long &p_refllValue) {p_refllValue = atoll(m_pszData);}
  void GetValue(float &p_reffValue) {p_reffValue = (float)atof(m_pszData);}
  void GetValue(double &p_refdValue) {p_refdValue = atof(m_pszData);}

public:
  void SetThis(
    const char *p_pszName, 
    char p_chType, 
    int p_iLength, 
    int p_iDecLen = 0, 
    int p_iOffset = 0, 
    const char *p_pszData = NULL, 
    int p_iDataLen = 0
  );

  char * GetData(void) const {return m_pszData;}
  int GetDataLen(void) const {return m_iDataLen;}

  char * GetName(void) const {return (char *)m_szName;}
  char GetType(void) const {return m_chType;}
  int GetLength(void) const {return m_iLength;}
  int GetDecLen(void) const {return m_iDecLen;}

private:
  char m_szName[FIELD_NAME_LEN + 1];
  char m_chType;
  int m_iLength;
  int m_iDecLen;
  int m_iOffset;

  int m_iDataLen;
  char *m_pszData;

  friend class CDbfRecord;
  friend class CDbFile;
};


//------------------------------------------------------------------------------
class CDbfRecord
{
public:
  CDbfRecord(void);
  CDbfRecord(
    CDbfField *p_paclDbfField, 
    int p_iFieldNum
  );
  ~CDbfRecord();

public:
  int SetFields(
    CDbfField *p_paclDbfField, 
    int p_iFieldNum
  );

  int GetFieldNum(void) const {return m_iFieldNum;}
  
  CDbfField & GetField(const char *p_pszFieldName);
  CDbfField & GetField(int p_pszFieldIndex);
  CDbfField & operator [](const char *p_pszFieldName) {return GetField(p_pszFieldName);}
  CDbfField & operator [](int p_pszFieldIndex) {return GetField(p_pszFieldIndex);}

  bool IsDeleted(void) const {return m_bIsDeleted;}

  CDbfField * GetNullField() {return &m_clNullField;}

private:
  static CDbfField m_clNullField;
  CDbfField *m_paclDbfField;
  int m_iFieldNum;

  bool m_bIsDeleted;

  friend class CDbFile;
};


//------------------------------------------------------------------------------
class CDbFile
{
public:
  CDbFile(void);
  CDbFile(
    CDbfRecord &p_refclDbfRecord, 
    const char *p_pszDbFile, 
    int p_iOpenMode = CDbFile::readOnly | CDbFile::shareOpen | CDbFile::autoLock | CDbFile::autoBuffer);
  ~CDbFile();

public:
  int Open(
    CDbfRecord &p_refclDbfRecord, 
    const char *p_pszDbFile, 
    int p_iOpenMode = CDbFile::readOnly | CDbFile::shareOpen | CDbFile::autoLock | CDbFile::autoBuffer);
  int Close(void);

  bool IsOpened(void) const {return m_iFd != -1 ? true : false;}
  int GetOpenMode(void) const {return m_iOpenMode;}

  // 获取缓冲区缓存记录数
  int GetBufferSize(void) const {return m_iBufferSize;}
  // 设置缓冲区缓存记录数（只有打开方式为：readBuffer或writeBuffer才具备缓冲能力）
  int SetBufferSize(int p_iBufferSize);
  
  int GetLockWaitTime(void) const {return m_iLockWaitTime;}
  int SetLockWaitTime(int p_iLockWaitTime) 
  {
    int iLockWaitTime = m_iLockWaitTime;
    m_iLockWaitTime = xsdk_max(0, p_iLockWaitTime);
    return iLockWaitTime;
  }

  int LockDb(void);
  int UnlockDb(void);
  
  int LockHead(void);
  int UnlockHead(void);
  
  int LockRecord(
    int p_iRecordNo, 
    int p_iRecNum = 1
  );
  int UnlockRecord(void);

  int GetRecordNum(bool p_bRefresh = false)
  {
    int iRetCode = XSDK_OK;
    if (p_bRefresh == true)
    {
      if ((iRetCode = LockHead()) == XSDK_OK)
      {
        iRetCode = ReadHead();
        UnlockHead();
      }
    }
    return iRetCode != XSDK_OK ? -1 : 
                       (m_iBufferOpt & append) == append ? (m_stHead.iRecordNum + m_iBufferRecNum) : (m_stHead.iRecordNum);
  }

  int CommitRecord(void);
  int AppendRecord(void);
  int UpdateRecord(int p_iRecordNo);
  int DeleteRecord(int p_iRecordNo);
  int SelectRecord(
    int p_iRecordNo, 
    bool p_bFilterSpace = true
  );

  int Truncate(void);
  int Create(CDbfRecord &p_refclDbfRecord, const char *p_pszDbFile);

private:
  int InitThis(void);

  int ReadHead(void);
  int WriteHead(void);
  int ReadFields(void);

  // 计算数据记录在数据文件中的偏移量
  long long RecordOffset(int p_iRecordNo)
  {return (long long)(m_stHead.siHeadLen) + (p_iRecordNo - 1LL) * (long long)(m_stHead.siRecordLen);}

  void FillRecBuff(void);

public:
  // 定义数据文件的操作类型
  enum OpenMode
  {
    exclOpen = 0x0001,
    shareOpen = 0x0002,

    readOnly = 0x0008,
    readWrite = 0x0010,

    autoLock = 0x0020,

    readBuffer = 0x0100,
    writeBuffer = 0x0200,
    autoBuffer = readBuffer | writeBuffer
  };
  // 数据缓冲区操作类型
  enum BufferOpt
  {
    select = 0x0001,
    update = 0x0002,
    append = 0x0004
  };

#ifdef __GNUC__ 
  #pragma pack(1)
#else
  #pragma pack(push, 1)
#endif
  struct HEAD
  {
    char chID;                                  // 数据库标志
    char chYear, chMonth, chDay;                // 数据库日期
    int iRecordNum;                             // 数据库记录数
    short siHeadLen;                            // 数据库头长度 = 头描述区 + 字段描述区
    short siRecordLen;                          // 记录宽度
    char szReserve[20];                         // 保留
  };

  struct FIELD
  {
    char szName[FIELD_NAME_LEN + 1];            // 字段名称
    char chType;                                // 字段类型
    short siOffset;                             // 字段偏移
    char szReserve01[2];                        // 保留
    unsigned char chLength;                     // 字段长度
    char chDecLen;                              // 小数长度
    char szReserve02[14];                       // 保留
  };
#ifdef __GNUC__
  #pragma pack()
#else
  #pragma pack(pop)
#endif

private:
  char m_szDbFile[256 + 1];
  int m_iOpenMode;

  int m_iLockWaitTime;                          // 锁定/解锁操作等待超时时间（单位：秒）
  bool m_bLockedDb;                             // 是否已数据文件级锁定
  bool m_bLockedHead;                           // 是否已锁定数据文件的头区域
  int m_iLockedRecNo;                          // 已锁定数据记录序号
  int m_iLockedRecNum;                         // 已锁定数据记录数

  //FILE *m_fpFileHandle;
  int m_iFd;
  struct HEAD m_stHead;
  CDbfRecord *m_pclDbfRecord;

  int m_iBufferOpt;                             // 数据缓冲区当前操作类型
  char *m_pszRecordBuff;                        // 数据缓冲区，分为：操作区、缓冲区
  long long m_llRecordBuffLen;                        // 数据缓冲区的空间大小，(m_stHead.siRecordLen * (m_lBufferSize + 1) + 1L)
  int m_iBufferSize;                           // 数据缓冲区最大缓存记录数
  int m_iBufferRecNo;                          // 数据缓冲区第一条记录序号
  int m_iBufferRecNum;                         // 数据缓冲区有效记录数，m_lBufferRecNum <= m_lBufferSize
};

END_NAMESPACE_XSDK

#endif  // __XSDK_DBF_H__
