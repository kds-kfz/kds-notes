#include<sys/wait.h>
#include <signal.h>
#include"klog.hpp"
#include"kipc.hpp"
#include"ksignal.hpp"

//信号注册
/*
 * signal 与 sigaction 的区别
 * signal 
 * 每次设置具体的信号处理回调函数后只能生效一次，随即将信号处理函数恢复为默认处理方式
 * 如果要多次相同方式处理某个信号,通常的做法是,在响应函数开始,再次调用 signal 设置
 * 缺点: 
 * (1) 在信号发生之后到信号处理程序中调用 signal 函数之间有一个时间窗口
 * 在此段时间中，可能发生另一次中断信号，第二个信号会造成执行默认动作，而对中断信号则是终止该进程
 * 这会导致前者信号没法处理
 * (2) 在进程不希望某种信号发生时，它不能关闭该信号
 *
 * sigaction
 * (1) 在信号处理程序被调用时，系统建立的新信号屏蔽字会自动包括正被递送的信号
 * 因此保证了在处理一个给定的信号时，如果这种信号再次发生，那么它会被阻塞，直到当前信号处理函数被处理完，然后才执行这一信号
 * (2) 响应函数设置后就一直有效，不会重置(即不会恢复默认处理方式)
 * (3) 对除 SIGALRM 以外的所有信号都企图设置 SA_RESTART 标志，于是被这些信号中断的系统调用 (read,write) 都能自动再起动
 * 不希望再起动由 SIGALRM 信号中断的系统调用的原因是希望对 I/O 操作可以设置时间限制
 * 
 * 所以希望能用相同方式处理信号的多次出现，最好用 sigaction，信号只出现并处理一次，可以用s ignal
 * */

/* int sigaction(int signum, const struct sigaction *act,struct sigaction *oldact);
 *
 * struct sigaction {
 *  void     (*sa_handler)(int);
 *  void     (*sa_sigaction)(int, siginfo_t *, void *);
 *  sigset_t   sa_mask;
 *  int        sa_flags;
 *  void     (*sa_restorer)(void);
 * }
 * sa_handler: 此参数和signal()的参数handler相同，代表新的信号处理函数
 * sa_mask: 用来设置在处理该信号时暂时将sa_mask 指定的信号集搁置
 * sa_flags: 用来设置信号处理的其他相关操作，下列的数值可用
 * SA_RESETHAND: 当调用信号处理函数时，将信号的处理函数重置为缺省值 SIG_DFL
 * SA_RESTART: 如果信号中断了进程的某个系统调用，则系统自动启动该系统调用
 * SA_NODEFER : 一般情况下， 当信号处理函数运行时，内核将阻塞该给定信号
 * 但是如果设置了 SA_NODEFER 标记， 那么在该信号处理函数运行时，内核将不会阻塞该信号
 * 
 * sa_flags: 可以是一下值的“按位或”组合
 * SA_RESTART：使被信号打断的系统调用自动重新发起
 * SA_NOCLDSTOP：使父进程在它的子进程暂停或继续运行时不会收到 SIGCHLD 信号
 * SA_NOCLDWAIT：使父进程在它的子进程退出时不会收到 SIGCHLD 信号，这时子进程如果退出也不会成为僵尸进程
 * SA_NODEFER：使对信号的屏蔽无效，即在信号处理函数执行期间仍能发出这个信号
 * SA_RESETHAND：信号处理之后重新设置为默认的处理方式
 * SA_SIGINFO：使用 sa_sigaction 成员而不是 sa_handler 作为信号处理函数
 * 
 * 备注:
 * SIG_DFL ：默认的处理方式是不理会这个信号，但是也不会丢弃子进行状态，所以如果不用wait，waitpid
 * SIG_IGN ：忽略的处理方式
 */

bool initSigProc(){
#if 0
    if(SIG_ERR == signal(SIGINT, SigHandlerProc)){
        ERROR_TLOG("signal SIGINT SigHandlerProc Error!\n");
        return false;
    }
    if(SIG_ERR == signal(SIGFPE, SigHandlerProc)){
        ERROR_TLOG("signal SIGFPE SigHandlerProc Error!\n");
        return false;
    }
    if(SIG_ERR == signal(SIGCHLD, SigHandlerProc)){
        ERROR_TLOG("signal SIGCHLD SigHandlerProc Error!\n");
        return false;
    }
#endif
    struct sigaction sa;
    sa.sa_handler = SigHandlerProc;
    //一般情况下，当信号处理函数运行时，内核将阻塞该给定信号
    //但如果设置了 SA_NODEFER 标记，那么在该信号处理函数运行时，内核不会阻塞该信号
    sa.sa_flags = SA_NODEFER;
    
    sigemptyset(&sa.sa_mask);
    //避免 zombie 的方法
    sa.sa_flags |= SA_NOCLDWAIT;
    
    int ret = sigaction(SIGCHLD, &sa, NULL);
    if(ret < 0){
        ERROR_TLOG("Error: 设置 SIGCHLD 信号处理方式失败!\n");
        return false;
    }
    ret = sigaction(SIGINT, &sa, NULL);
    if(ret < 0){
        ERROR_TLOG("Error: 设置 SIGINT 信号处理方式失败!\n");
        return false;
    }

    ret = sigaction(SIGFPE, &sa, NULL);
    if(ret < 0){
        ERROR_TLOG("Error: 设置 SIGFPE 信号处理方式失败!\n");
        return false;
    }
    ret = sigaction(SIGSEGV, &sa, NULL);
    if(ret < 0){
        ERROR_TLOG("Error: 设置 SIGSEGV 信号处理方式失败!\n");
        return false;
    }
    ret = sigaction(SIGBUS, &sa, NULL);
    if(ret < 0){
        ERROR_TLOG("Error: 设置 SIGBUS 信号处理方式失败!\n");
        return false;
    }
    ret = sigaction(SIGABRT, &sa, NULL);
    if(ret < 0){
        ERROR_TLOG("Error: 设置 SIGABRT 信号处理方式失败!\n");
        return false;
    }
    /*
    //忽略该信号
    //SIG_IGN 这个符号表示忽略该信号，进程会忽略该信号
    sa.sa_handler = SIG_IGN;
    ret = sigaction(SIGPIPE, &sa, NULL);
    if(ret < 0){
        ERROR_TLOG("Error: 设置 SIGPIPE 信号处理方式失败!\n");
        return false;
    }
    */
    return true;
}

//信号回调
void SigHandlerProc(int signo){
    if(signo == SIGCHLD){
        int status = 0;
        pid_t pid = 0;
        while((pid = waitpid(-1, &status, WNOHANG)) > 0){
            INFO_TLOG("the child exit status is [%s], pid = [%d], signo = SIGCHLD\n", WEXITSTATUS(status), pid);
        }
    }else if(signo == SIGINT){
        INFO_TLOG("SigHandlerProc: signo = SIGINT\n");
        KIPC::delsem();
        exit(0);
    }else if(signo == SIGFPE || signo == SIGSEGV || signo == SIGBUS || signo == SIGABRT){
        string errmsg;
        errmsg.clear();
        if(signo == SIGFPE){
            errmsg.append("SIGFPE");
        }else if(signo == SIGSEGV){
            errmsg.append("SIGSEGV");
        }else if(signo == SIGBUS){
            errmsg.append("SIGBUS");
        }else if(signo == SIGABRT){
            errmsg.append("SIGABRT");
        }
        INFO_TLOG("SigHandlerProc: signo = [%s]\n", errmsg.c_str());
    }
}

