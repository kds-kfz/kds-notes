#if !defined(__xsdk_socket_h__)
#define __xsdk_socket_h__

#include "xsdk_define.h"

#if defined(OS_IS_WINDOWS)
  #include <winsock2.h>
  #include <wsipx.h>
  #include <wsnwlink.h>
  typedef int                   socklen;
#else
  #include <fcntl.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <sys/types.h>
  #include <sys/socket.h>
  #include <sys/select.h>
  #include <sys/un.h>
  #include <netdb.h>
  #include <unistd.h>
  #include <errno.h>

  typedef int                   SOCKET;
  typedef socklen_t             socklen;
  #define closesocket           close

  #if !defined(INVALID_SOCKET)
  #define INVALID_SOCKET        (-1)
  #endif

  #if !defined(SOCKET_ERROR)
  #define SOCKET_ERROR          (-1)
  #endif
#endif


#ifdef  __cplusplus
extern "C"
{
#endif

#define XSDK_SOCKET_TCP         1
#define XSDK_SOCKET_UDP         2
#define XSDK_SOCKET_SPX         3
#define XSDK_SOCKET_SHM         5

#define XSDK_SOCKET_ENRECV      1
#define XSDK_SOCKET_ENSEND      2

#define DSP2HEX(p_pszHex, p_pszDsp, p_iLen) \
{\
  for (int iIndex = 0; iIndex < p_iLen; iIndex++)\
  {\
    p_pszHex[iIndex] = p_pszDsp[iIndex*2] < 0x39 ? p_pszDsp[iIndex * 2] - 0x30 : p_pszDsp[iIndex * 2] - 0x41 + 0x0A;\
    p_pszHex[iIndex] = p_pszHex[iIndex] << 4;\
    p_pszHex[iIndex] += p_pszDsp[iIndex * 2 + 1] < 0x39 ? p_pszDsp[iIndex * 2 + 1] - 0x30 : p_pszDsp[iIndex * 2 + 1] - 0x41 + 0x0A;\
  }\
}

#define HEX2DSP(p_pszDsp, p_pszHex, p_iLen) \
{\
  int iIndex = 0, iByte = 0;\
  for (iIndex = 0; iIndex < p_iLen; iIndex++)\
  {\
    iByte = (p_pszHex[iIndex] & 0xF0) >> 4;\
    p_pszDsp[iIndex * 2] = iByte > 0x09 ? iByte + 0x41 - 0x0A : iByte + 0x30;\
    iByte = (p_pszHex[iIndex] & 0x0F);\
    p_pszDsp[iIndex * 2 + 1] = iByte > 0x09 ? iByte + 0x41 - 0x0A : iByte + 0x30;\
  }\
}


BGN_NAMESPACE_XSDK

struct ST_SOCKOPT
{
  INT iTcpNoDelay;
  INT iSoSndBuf;
  INT iSoRcvBuf;
};

BOOL SocketInit(
  WORD p_wdVerReq = MAKEWORD(0x02, 0x02)
);

BOOL SocketUninit(void);

//------------------------------------------------------------------------------
// 功能描述：
//     服务端调用，创建套接字，并在指定地址和端口侦听客户端的连接请求
// 参数说明：
//     p_iProtocol[in]
//         协议类型
//     p_pszAddress[in]
//         侦听地址
//             TCP/IP地址格式：xxx.xxx.xxx.xxx，如：192.168.1.1
//             SPX/IPX地址格式：(8字节的)网段,(12字节)地址，如：00000001,001302CA4378
//     p_nPort[in]
//         侦听端口，十进制
//     p_iBackLog[in]
//         等待连接队列的最大长度
//     p_pstSockOpt[in]
//         setsockopt调用的套接字选项
// 返回说明：
//     SOCKET                   创建成功的套接字
SOCKET SocketListen(
  INT p_iProtocol,
  LPCSTR p_pszAddress,
  UINT p_nPort,
  INT p_iBackLog,
  struct ST_SOCKOPT *p_pstSockOpt = NULL
);


//------------------------------------------------------------------------------
// 功能描述：
//     服务端调用，接受客户端连接请求，并返回连接套接字
// 参数说明：
//     p_hSocket[in]
//         监听套接字，SocketListen调用返回的套接字
//     p_pstClientAddr[out]
//         监听连接成功，返回客户端地址信息
//     p_piAddrLen[out]
//         p_pstClientAddr存储大小
// 返回说明：
//     SOCKET                   连接成功的套接字
SOCKET SocketAccept(
  SOCKET p_hSocket,
  struct sockaddr *p_pstClientAddr,
  INT *p_piAddrLen
);


//------------------------------------------------------------------------------
// 功能描述：客户端调用，创建套接字，并与服务端连接
// 入参说明：
//     p_iProtocol[in]
//         协议类型
//     p_pszAddress[in]
//         连接地址，与SocketListen调用的地址格式相同
//     p_nPort[in]
//         连接端口，十进制
// 返回说明：
//     SOCKET                   连接成功的套接字
SOCKET SocketConnect(
  INT p_iProtocol,
  LPCSTR p_pszAddress,
  UINT p_nPort
);


//------------------------------------------------------------------------------
// 功能描述：
//     检查套接字：是否能接收信息，是否能发送信息等
// 参数说明：
//     p_hSocket[in]
//         套接字
//     p_nOptional[in]
//         检查选项：XSDK_SOCKET_ENRECV 是否能接收信息，
//                   SOCKET_ENSEND 是否能发送信息
//     p_nTimeout[in]
//         超时时间，单位：秒
// 返回说明：
//     XSDK_OK     可以操作
//     XSDK_KO     检测出错
//     XSDK_KO - 1 检测超时
INT SocketCheck(
  SOCKET p_hSocket,
  UINT p_nOptional,
  UINT p_nTimeout
);


//------------------------------------------------------------------------------
// 功能描述：
//     发送信息
// 参数说明：
//     p_hSocket[in]
//         已连接成功的套接字
//     p_pbyData[in]
//         发送信息缓冲区
//     p_nBytes[in]
//         发送信息字节数
//     p_nTimeout[in]
//         超时时间，单位：秒
// 返回说明：
//     >=0         成功发送信息的字节数
//     XSDK_KO     发送信息失败
//     XSDK_KO - 1 发送信息超时
INT SocketSend(
  SOCKET p_hSocket,
  CONST BYTE *p_pbyData,
  UINT p_nBytes,
  UINT p_nTimeout
);


//------------------------------------------------------------------------------
// 功能描述：
//     接收信息
// 参数说明：
//     p_hSocket[in]
//         已连接成功的套接字
//     p_pbyData[in]
//         接收信息缓冲区
//     p_nBytes[in]
//         接收信息缓冲区大小
//     p_nTimeout[in]
//         超时时间，单位：秒
// 返回说明：
//     >=0         成功接收信息的字节数
//     XSDK_KO     接收信息失败
//     XSDK_KO - 1 接收信息超时
INT SocketReceive(
  SOCKET p_hSocket,
  BYTE *p_pbyData,
  UINT p_nBytes,
  UINT p_nTimeout
);


//------------------------------------------------------------------------------
// 功能描述：
//     关闭套接字
// 参数说明：
//     p_hSocket[in]
//         套接字
// 返回说明：
//     = 0 成功关闭
//     SOCKET_ERROR
INT SocketClose(
  SOCKET p_hSocket
);


#ifdef __cplusplus
}
#endif

END_NAMESPACE_XSDK

#endif  // __xsdk_socket_h__
