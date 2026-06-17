#pragma once

#include <windows.h>

enum EN_S_NOTIFY_TYPE
{
    EN_S_TYPE_CLOSE = 0,
    EN_S_TYPE_CONNECTED,
    EN_S_TYPE_ERROR,
    EN_S_TYPE_DATA,
};

// 旧 m_hSocket.dll 的服务句柄类型，新版内部映射到 SocketServer 实例。
typedef struct ST_TAG_HS* HS;
// 旧 m_hSocket.dll 的客户端句柄类型，调用方只比较和回传，不直接解引用。
typedef struct ST_TAG_HCLIENT* HCLIENT;

// SocketServer 兼容层回调签名，保持旧 TCP 推送模块 ABI 不变。
typedef void(WINAPI* S_NOTIFY_PROC)(
    HS p_hHandle,
    HCLIENT p_hUser,
    EN_S_NOTIFY_TYPE p_enType,
    const void* p_pData,
    int p_iDataLen,
    const char* p_szIp,
    unsigned short p_uPort,
    void* p_pParam);

typedef bool(WINAPI* S_TRAVEL_FUNC)(HS p_hHandle, HCLIENT p_hUser, void* p_pParam);

// 创建 TCP 服务并注册旧式通知回调。
extern "C" HS s_create(
    unsigned short p_uPort,
    int p_iRBufLen,
    int p_iMaxConnectNum,
    int p_iMaxConnectNumPerIp,
    int p_iMaxIdleSecond,
    S_NOTIFY_PROC p_pfnCallback,
    void* p_pParam,
    int p_iHandleThreadNum = 0,
    char* p_szErr = nullptr);

// 向指定客户端发送数据，内部转发到新版 SocketServer。
extern "C" void s_send(HS p_hHandle, HCLIENT p_hUser, char* p_pData, int p_iDataLen);
extern "C" void s_travel(HS p_hHandle, S_TRAVEL_FUNC p_pfnTravel, void* p_pParam);
extern "C" void s_close(HS p_hHandle, HCLIENT p_hUser);
// 停止服务并释放兼容句柄。
extern "C" void s_destroy(HS p_hHandle);
extern "C" void s_setdata(HS p_hHandle, HCLIENT p_hUser, DWORD p_dwData);
extern "C" void s_cleardata(HS p_hHandle, HCLIENT p_hUser);
extern "C" bool s_getdata(HS p_hHandle, HCLIENT p_hUser, DWORD* p_pdwData);
extern "C" int s_compare(HCLIENT p_hFirst, HCLIENT p_hSecond);
