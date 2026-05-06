#if !defined(__xsdk_hashtable_h__)
#define __xsdk_hashtable_h__

#include <string>
using namespace std;
#include <xsdk_define.h>

//BGN_NAMESPACE_XSDK

template <class _CKey>
class CHash
{
public:
  size_t Value(_CKey p_clKey) const {return p_clKey;}
  int Compare(const _CKey &p_refclLeft, const _CKey &p_refclRight) const
  {
    return (p_refclLeft > p_refclRight ? 1 :
            p_refclLeft < p_refclRight ? -1 : 0);
  }
};

template<>
class CHash<const char *>
{
public:
  size_t Value(const char *p_pszKey) const
  {
    size_t llValue = 0;
    for (llValue = 0; p_pszKey[0] != 0x00; llValue = llValue * 31 + *p_pszKey++);
    return llValue;
  }
  int Compare(const char *p_pszLeft, const char *p_pszRight) const
  {
    return strcmp(p_pszLeft, p_pszRight);
  }
};

template<>
class CHash<string>
{
public:
  size_t Value(const string &p_refstrKey) const
  {
    size_t llValue = 0;
    for (unsigned int uiIndex = 0; uiIndex < p_refstrKey.size(); ++uiIndex)
    {
      llValue = llValue * 31 + p_refstrKey[uiIndex];
    }
    return llValue;
  }
  int Compare(const string &p_refstrLeft, const string &p_refstrRight) const
  {
    return (p_refstrLeft > p_refstrRight ? 1 :
            p_refstrLeft < p_refstrRight ? -1 : 0);
  }
};

template<class _CKey, class _CVal>
struct _HASHTABLE_NODE
{
  _CKey clKey;
  _CVal clVal;

  DWORD dwNextNodeIdx;
};

template<
  class _CKey, 
  class _CVal,
  class _CHash = CHash<_CKey> >
class CHashTable
{
protected:
  // 内部使用的哈希表节点定义
/*  template<class _CKey, class _CVal>
  struct _HASHTABLE_NODE
  {
    _CKey clKey;
    _CVal clVal;

    DWORD dwNextNodeIdx;
  };
*/
  typedef _HASHTABLE_NODE<_CKey, _CVal> HASHTABLE_NODE;

  struct HASHTABLE_INFO
  {
    DWORD dwTableSize;          // 哈希表空间大小
    DWORD dwBucketSize;         // 桶容量（索引容量）
    DWORD dwNodeSize;           // 节点容量
    DWORD dwNodeNum;            // 已存储节点数量
  };

  // 计算哈希表空间大小的宏函数
  #define GET_HASHTABLE_SIZE(dwBucketSize, dwNodeSize) \
          (sizeof(HASHTABLE_INFO) + sizeof(DWORD) * (dwBucketSize) + sizeof(HASHTABLE_NODE) * (dwNodeSize))
  // 无效索引值
  #define INVALID_INDEX         0xFFFFFFFF

public:
  CHashTable(void)
  : m_pbyHTable(NULL)
  {
    InitThis();
  }

  //----------------------------------------------------------------------------
  // 功能描述：创建哈希表的构造函数
  // 入参说明：
  //     [in]  p_dwBucketSize   桶（桶：即表索引）容量，此类会调用GetNearPrimeNbr方法修正桶容量
  //     [in]  p_dwNodeSize     节点容量。
  //     [in]  p_pvdTableBuff   外部已经分配好的哈希表空间（类不自动释放），
  //                            若为NULL，则类内部自动分配（类自动释放）。
  //     [in]  p_dwBufferSize   p_pvdTableBuff的容量。
  // 返回说明：
  //     VOID
  CHashTable(
    DWORD p_dwBucketSize, 
    DWORD p_dwNodeSize, 
    void *p_pvdTableBuff = NULL, 
    DWORD p_dwBufferSize = 0
  ) : m_pbyHTable(NULL)
  {
    Create(p_dwBucketSize, p_dwNodeSize, p_pvdTableBuff, p_dwBufferSize);
  }

  //----------------------------------------------------------------------------
  // 功能描述：装载哈希表的构造函数
  // 入参说明：
  //     [in]  p_pvdTableBuff  已存在的哈希表空间，哈希表空间存储格式如下：
  //                            {
  //                              DWORD dwTableSize;            // 哈希表空间大小
  //                              DWORD dwBucketSize;           // 桶容量
  //                              DWORD dwNodeSize;             // 节点容量
  //                              DWORD dwNodeNum;              // 已存储节点数量
  //                              {
  //                                ...                         // 数据区（桶 + 节点）
  //                              }
  //                            }
  // 返回说明：
  //     
  CHashTable(void *p_pvdTableBuff)
  : m_pbyHTable(NULL)
  {
    Load(p_pvdTableBuff);
  }

  ~CHashTable()
  {
    InitThis();
  }

  //----------------------------------------------------------------------------
  // 功能描述：创建哈希表
  // 入参说明：
  //     参见创建哈希表的构造函数
  // 返回说明：
  //     XSDK_OK
  int Create(
    DWORD p_dwBucketSize, 
    DWORD p_dwNodeSize, 
    void *p_pvdTableBuff = NULL, 
    DWORD p_dwBufferSize = 0
  )
  {
    int iRetCode = XSDK_OK;

    DWORD dwBucketSize = 0, dwNodeSize = 0, dwTableSize = 0;
    BYTE *pbyTableBuff = (BYTE *)p_pvdTableBuff;

    InitThis();

    dwBucketSize = GetNearPrimeNbr(p_dwBucketSize);
    dwNodeSize = p_dwNodeSize;
    dwTableSize = GET_HASHTABLE_SIZE(dwBucketSize, dwNodeSize);

    if (pbyTableBuff == NULL)
    {
      m_pbyHTable = new BYTE[dwTableSize];
      if (m_pbyHTable == NULL)
      {
        iRetCode = XSDK_KO;
        goto __end;
      }

      pbyTableBuff = m_pbyHTable;
    }
    else if (p_dwBufferSize < dwTableSize)
    {
      iRetCode = XSDK_KO;
      goto __end;
    }

    memset((void *)pbyTableBuff, 0x00, sizeof(BYTE) * dwTableSize);
    m_pstHTable = (HASHTABLE_INFO *)pbyTableBuff;
    m_pdwBuckets = (DWORD *)(pbyTableBuff + sizeof(HASHTABLE_INFO));
    m_pstHTableNode = (HASHTABLE_NODE *)(pbyTableBuff + sizeof(HASHTABLE_INFO) + sizeof(DWORD) * dwBucketSize);
    
    m_pstHTable->dwTableSize = dwTableSize;
    m_pstHTable->dwBucketSize = dwBucketSize;
    m_pstHTable->dwNodeSize = dwNodeSize;
    memset((void *)m_pdwBuckets, 0xFF, sizeof(DWORD) * dwBucketSize);
  
  __end:
    return iRetCode;
  }

  //----------------------------------------------------------------------------
  // 功能描述：装载哈希表
  // 入参说明：
  //     参见装载哈希表的构造函数
  // 返回说明：
  //     
  int Load(void *p_pvdTableBuff)
  {
    int iRetCode = XSDK_OK;

    BYTE *pbyTableBuff = (BYTE *)p_pvdTableBuff;
    if (pbyTableBuff == NULL)
    {
      iRetCode = XSDK_KO;
      goto __end;
    }
    InitThis();

    m_pstHTable = (HASHTABLE_INFO *)pbyTableBuff;
    m_pdwBuckets = (DWORD *)(pbyTableBuff + sizeof(HASHTABLE_INFO));
    m_pstHTableNode = (HASHTABLE_NODE *)(pbyTableBuff + sizeof(HASHTABLE_INFO) + sizeof(DWORD) * m_pstHTable->dwBucketSize);
    if (m_pstHTable->dwTableSize != GET_HASHTABLE_SIZE(m_pstHTable->dwBucketSize, m_pstHTable->dwNodeSize))
    {
      InitThis();
      iRetCode = XSDK_KO;
      goto __end;
    }

  __end:
    return iRetCode;
  }

  DWORD GetTableSize(void)
  {
    return m_pstHTable == NULL ? 0 : m_pstHTable->dwTableSize;
  }

  DWORD GetBucketCount(void)
  {
    return m_pstHTable == NULL ? 0 : m_pstHTable->dwBucketSize;
  }

  DWORD GetNodeSize(void)
  {
    return m_pstHTable == NULL ? 0 : m_pstHTable->dwNodeSize;
  }

  DWORD GetNodeCount(void)
  {
    return m_pstHTable == NULL ? 0 : m_pstHTable->dwNodeNum;
  }

  int Close(void)
  {
    int iRetCode = XSDK_OK;
    
    InitThis();
    
    return iRetCode;
  }

public:
  int SetElement(
    const _CKey p_clKey, 
    const _CVal p_clVal
  )
  {
    int iRetCode = XSDK_OK;

    _CHash clHash;
    DWORD dwKeyIndex = 0;
    DWORD dwNodeIdx = 0, dwPreNodeIdx = 0;

    if (m_pstHTable == NULL || m_pstHTableNode == NULL || m_pdwBuckets == NULL)
    {
      iRetCode = XSDK_KO;
      goto __end;
    }

    dwKeyIndex = clHash.Value(p_clKey) % (m_pstHTable->dwBucketSize);
    dwNodeIdx = dwPreNodeIdx = m_pdwBuckets[dwKeyIndex];
    if (dwNodeIdx == INVALID_INDEX)
    {
      // 新的元素
      if ((m_pstHTable->dwNodeNum) >= (m_pstHTable->dwNodeSize))
      {
        iRetCode = XSDK_KO;
        goto __end;
      }
      m_pstHTableNode[m_pstHTable->dwNodeNum].clKey = p_clKey;
      m_pstHTableNode[m_pstHTable->dwNodeNum].clVal = p_clVal;
      m_pstHTableNode[m_pstHTable->dwNodeNum].dwNextNodeIdx = INVALID_INDEX;
      m_pdwBuckets[dwKeyIndex] = (m_pstHTable->dwNodeNum);
      (m_pstHTable->dwNodeNum)++;
      goto __end;
    }
    else
    {
      while (dwNodeIdx != INVALID_INDEX && clHash.Compare(p_clKey, m_pstHTableNode[dwNodeIdx].clKey) > 0)
      {
        dwPreNodeIdx = dwNodeIdx;
        dwNodeIdx = m_pstHTableNode[dwNodeIdx].dwNextNodeIdx;
      }

      if (dwNodeIdx != INVALID_INDEX && clHash.Compare(p_clKey, m_pstHTableNode[dwNodeIdx].clKey) == 0)
      {
        // 找到相等的元素，则赋值
        m_pstHTableNode[dwNodeIdx].clVal = p_clVal;
        goto __end;
      }
      else if (dwNodeIdx == INVALID_INDEX || clHash.Compare(p_clKey, m_pstHTableNode[dwNodeIdx].clKey) < 0)
      {
        if ((m_pstHTable->dwNodeNum) >= (m_pstHTable->dwNodeSize))
        {
          iRetCode = XSDK_KO;
          goto __end;
        }
        m_pstHTableNode[m_pstHTable->dwNodeNum].clKey = p_clKey;
        m_pstHTableNode[m_pstHTable->dwNodeNum].clVal = p_clVal;
        m_pstHTableNode[m_pstHTable->dwNodeNum].dwNextNodeIdx = dwNodeIdx;
        if (dwNodeIdx == m_pdwBuckets[dwKeyIndex])
        {
          // 在桶的链头插入新值，则需要修改桶里的下标
          m_pdwBuckets[dwKeyIndex] = m_pstHTable->dwNodeNum;
        }
        else
        {
          m_pstHTableNode[dwPreNodeIdx].dwNextNodeIdx = (m_pstHTable->dwNodeNum);
        }
        (m_pstHTable->dwNodeNum)++;
        goto __end;
      }
    }

  __end:
    return iRetCode;
  }

  int GetElement(
    const _CKey p_clKey, 
    _CVal &p_refclVal
  )
  {
    int iRetCode = XSDK_OK;

    _CHash clHash;
    DWORD dwKeyIndex = 0;
    DWORD dwNodeIdx = 0;

    if (m_pstHTable == NULL || m_pstHTableNode == NULL || m_pdwBuckets == NULL)
    {
      iRetCode = XSDK_KO;
      goto __end;
    }

    dwKeyIndex = clHash.Value(p_clKey) % (m_pstHTable->dwBucketSize);
    dwNodeIdx = m_pdwBuckets[dwKeyIndex];
    while (dwNodeIdx != INVALID_INDEX && clHash.Compare(p_clKey, m_pstHTableNode[dwNodeIdx].clKey) > 0)
    {
      dwNodeIdx = m_pstHTableNode[dwNodeIdx].dwNextNodeIdx;
    }
    if (dwNodeIdx == INVALID_INDEX || clHash.Compare(p_clKey, m_pstHTableNode[dwNodeIdx].clKey) != 0)
    {
      iRetCode = XSDK_NO_DATA;
      goto __end;
    }
    
    // 找到相应的元素
    p_refclVal = m_pstHTableNode[dwNodeIdx].clVal;

  __end:
    return iRetCode;
  }

  int DelElement(
    const _CKey p_clKey, 
    const _CVal p_clVal
  )
  {
    int iRetCode = XSDK_OK;

    _CHash clHash;
    DWORD dwKeyIndex = 0;
    DWORD dwPreNodeIdx = 0, dwNodeIdx = 0;
    
    if (m_pstHTable == NULL || m_pstHTableNode == NULL || m_pdwBuckets == NULL)
    {
      iRetCode = XSDK_KO;
      goto __end;
    }

    dwKeyIndex = clHash.Value(p_clKey) % (m_pstHTable->dwBucketSize);
    dwPreNodeIdx = dwNodeIdx = m_pdwBuckets[dwKeyIndex];
    while (dwNodeIdx != INVALID_INDEX && clHash.Compare(p_clKey, m_pstHTableNode[dwNodeIdx].clKey) > 0)
    {
      dwPreNodeIdx = dwNodeIdx;
      dwNodeIdx = m_pstHTableNode[dwNodeIdx].dwNextNodeIdx;
    }
    if (dwNodeIdx == INVALID_INDEX || clHash.Compare(p_clKey, m_pstHTableNode[dwNodeIdx].clKey) != 0)
    {
      iRetCode = XSDK_NO_DATA;
      goto __end;
    }

    if (dwNodeIdx == m_pdwBuckets[dwKeyIndex])
    {
      m_pdwBuckets[dwKeyIndex] = INVALID_INDEX;
    }
    else if (m_pstHTableNode[dwNodeIdx].dwNextNodeIdx == INVALID_INDEX)
    {
      m_pstHTableNode[dwPreNodeIdx].dwNextNodeIdx = INVALID_INDEX;
    }
    else
    {
      m_pstHTableNode[dwPreNodeIdx].dwNextNodeIdx = m_pstHTableNode[dwNodeIdx].dwNextNodeIdx;
    }
    
    // 修正节点中链指针的索引值
    if (dwNodeIdx < (m_pstHTable->dwNodeNum - 1))
    {
      memcpy(m_pstHTableNode + dwNodeIdx, 
             m_pstHTableNode + dwNodeIdx + 1, 
             sizeof(HASHTABLE_NODE) * (m_pstHTable->dwNodeNum - 1 - dwNodeIdx));

      for (DWORD dwIndex = dwNodeIdx; dwIndex < m_pstHTable->dwNodeNum; dwIndex++)
      {
        if (m_pstHTableNode[dwIndex].dwNextNodeIdx != INVALID_INDEX 
          && m_pstHTableNode[dwIndex].dwNextNodeIdx > dwNodeIdx)
        {
          (m_pstHTableNode[dwIndex].dwNextNodeIdx)--;
        }
      }
    }
    // 修正桶内索引值
    for (DWORD dwIndex = 0; dwIndex < m_pstHTable->dwBucketSize; dwIndex++)
    {
      if (m_pdwBuckets[dwIndex] != INVALID_INDEX && m_pdwBuckets[dwIndex] > dwNodeIdx)
      {
        m_pdwBuckets[dwIndex]--;
      }
    }
    // 末位节点清零
    memset((void *)m_pstHTableNode + m_pstHTable->dwNodeNum - 1, 0x00, sizeof(HASHTABLE_NODE));
    (m_pstHTable->dwNodeNum)--;

  __end:
    return iRetCode;
  }

  int Cleanall(void)
  {
    int iRetCode = XSDK_OK;

    if (m_pstHTable == NULL || m_pstHTableNode == NULL || m_pdwBuckets == NULL)
    {
      iRetCode = XSDK_KO;
      goto __end;
    }

    m_pstHTable->dwNodeNum = 0;
    memset((void *)m_pdwBuckets, 0xFF, sizeof(DWORD) * (m_pstHTable->dwBucketSize));
    memset((void *)m_pstHTableNode, 0x00, sizeof(HASHTABLE_NODE) * (m_pstHTable->dwNodeSize));

    return iRetCode;
  }

public:
  //----------------------------------------------------------------------------
  // 功能描述：从素数数组中获取与指定数值最接近的素数
  //           主要用于确定桶（哈希表索引）的空间大小
  // 入参说明：
  //     [in]  p_dwNumber       指定数值
  // 返回说明：
  //     最接近的素数
  DWORD GetNearPrimeNbr(DWORD p_dwNumber)
  {
    static const DWORD PRIME_LIST[] =
    {
      53ul,         97ul,         193ul,       389ul,       769ul,      
      1543ul,       3079ul,       6151ul,      12289ul,     24593ul,    
      49157ul,      98317ul,      196613ul,    393241ul,    786433ul,   
      1572869ul,    3145739ul,    6291469ul,   12582917ul,  25165843ul, 
      50331653ul,   100663319ul,  201326611ul, 402653189ul, 805306457ul,
      1610612741ul, 3221225473ul, 4294967291ul  
    };
    
    DWORD dwNumber = 0;
    int iCount = sizeof(PRIME_LIST) / sizeof(DWORD);

    for (int iIndex = 0; iIndex < iCount; iIndex++)
    {
      if (p_dwNumber < PRIME_LIST[iIndex])
      {
        dwNumber = PRIME_LIST[iIndex];
        break;
      }
    }
    return dwNumber;
  }

protected:
  void InitThis(void)
  {
    m_pstHTable = NULL;
    m_pdwBuckets = NULL;
    m_pstHTableNode = NULL;

    if (m_pbyHTable != NULL)
    {
      delete []m_pbyHTable;
      m_pbyHTable = NULL;
    }
  }

protected:
  HASHTABLE_INFO *m_pstHTable;
  HASHTABLE_NODE *m_pstHTableNode;
  DWORD *m_pdwBuckets;

  // 类内部哈希表空间指针
  BYTE *m_pbyHTable;
};

//END_NAMESPACE_XSDK

#endif  // __xsdk_hashtable_h__
