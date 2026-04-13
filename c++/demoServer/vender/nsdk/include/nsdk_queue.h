#ifndef _NSDK_QUEUE_H_
#define _NSDK_QUEUE_H_

using namespace std;
#include <queue>
#include "xsdk_rwlock.h"

BGN_NAMESPACE_XSDK

template< class _QUEUE_DATA >
class CQueue
{
public:
  CQueue()
  {
    m_mutex.Create();
  }

  ~CQueue()
  {
    m_mutex.WriteLock();
    m_clQueue.clear();
    m_mutex.WriteUnlock();
    m_mutex.Close();
  }

  //---------------------------------------------------------
  // 函数名称：AddToTail
  // 函数描述：在队列尾增加一个元素
  // 其他说明：(空)
  // 入参说明： [in] const _QUEUE_DATA & p_stQueueData   队列元素数据
  // 返回结果： bool     false  失败   true  成功
  //--------------------------------------------------------
  bool AddToTail( const _QUEUE_DATA & p_stQueueData )
  {
    m_mutex.WriteLock();
    m_clQueue.push_back( p_stQueueData );
    m_mutex.WriteUnlock();
    return true;
  }

  //---------------------------------------------------------
  // 函数名称：RemoveFromHead
  // 函数描述：从队头中移出对列
  // 其他说明：(空)
  // 入参说明： [out] _QUEUE_DATA & p_refstQueueData   输出队列队头元素数据
  // 返回结果： bool     false  失败   true  成功
  //--------------------------------------------------------
  bool RemoveFromHead( _QUEUE_DATA & p_refstQueueData )
  {
    bool bRet = false;
    m_mutex.WriteLock();
    if( !m_clQueue.empty() ) 
    {
      p_refstQueueData = m_clQueue.front();
      m_clQueue.pop_front();
      bRet = true;
    }
    m_mutex.WriteUnlock();
    return bRet;
  }

  //---------------------------------------------------------
  // 函数名称：GetCount
  // 函数描述：获得队列中数据元素个数
  // 其他说明：(空)
  // 入参说明：(空)
  // 返回结果： int     返回队列中元素个数
  //--------------------------------------------------------
  int GetCount()
  {
    int iCount = 0;
    m_mutex.ReadLock();
    iCount = m_clQueue.size();
    m_mutex.ReadUnlock();
    return iCount;
  }

  //---------------------------------------------------------
  // 函数名称：IsEmpty
  // 函数描述：判定队列是否为空
  // 其他说明：(空)
  // 入参说明：(空)
  // 返回结果： bool  true 队列为空  false 队列不为空
  //--------------------------------------------------------
  bool IsEmpty()
  {
    bool bRet = false;
    m_mutex.ReadLock();
    if( 0 == m_clQueue.size() )
    {
      bRet = true;
    }
    m_mutex.ReadUnlock();
    return bRet;
  }

  //---------------------------------------------------------
  // 函数名称：ClearAll
  // 函数描述：清空队列中所有元素
  // 其他说明：(空)
  // 入参说明：(空)
  // 返回结果： bool  true 成功  false 失败
  //--------------------------------------------------------
  bool ClearAll()
  {
    m_mutex.WriteLock();
    m_clQueue.clear();
    m_mutex.WriteUnlock();
    return true;
  }

  void RefLock()
  {
	  m_mutex.WriteLock();
  }
  void RefUnlock()
  {
	  m_mutex.WriteUnlock();
  }
  deque<_QUEUE_DATA> &Reffer()
  {
	  return m_clQueue;
  }

private:
  deque< _QUEUE_DATA > m_clQueue;  //队列容器
  CRWLock m_mutex; //锁

};

END_NAMESPACE_XSDK

#endif