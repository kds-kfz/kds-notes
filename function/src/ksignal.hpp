#ifndef __KSIGNAL_H__
#define __KSIGNAL_H__

/************************************* 标 签 *************************************
 * 作者：kfz
 * 日期：2019-09-18
 * 描述：常用信号，有以下几种
 * SIGINT 程序终止(interrupt)信号，在用户键入INTR字符(通常是ctrl + c)是发出, 用于通知前台进程组终止进程
 * SIGCHLD：进程终止或停止时，将该信号发给其父进程
 * SIGBUS：硬件故障，或是没有足够的 RAM，malloc() 返回失败
 * SIGSEGV：试图访问未分配给自己的内存，试图往没有权限的内存地址写数据
 * (例如: 访问的内存已经释放, 访问的内存不存在或越界, 或试图对仅有毒隐社区与进行写操作)
 * SIGABRT：调用abort函数产生信号
 * SIGFPE：错误的算术操作，例如: 整数除以零、无效浮点操作等
 * SIGPIPE：当想已经关闭的socket发送数据时，产生此信号，默认处理是终止程序
 * 本文件就是针对常用的 IPC 信号进行注册，当进程触发这些信号从而相应处理
 * 设计理念：更好的管理信号，让系统更加稳定，资源合理调配
 * 加入守护进程
 **********************************************************************************/
//信号注册,  由于信号较多，每个信号有自己的处理函数，在此考虑使用函数指针

typedef std::string (*FuncHandle)();
typedef std::map<std::string, FuncHandle> FuncHandleMap;
static FuncHandleMap funcHandleMap;

//信号注册
bool initSigProc();
//信号回调
void SigHandlerProc(int signo);
//注册信号回调函数
void installHandle(string functionName, FuncHandle h);
//信号转 string
string SigToString(int signo);

//回调函数体
string handler_SIGCHLD();
string handler_SIGINT();
string handler_SIGFPE();
string handler_SIGSEGV();
string handler_SIGBUS();
string handler_SIGABRT();

#endif /* KSIGNAL */
