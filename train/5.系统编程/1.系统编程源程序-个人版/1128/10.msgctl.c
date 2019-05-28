#include<stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <stdlib.h>
//10.msgrcv.c
#if 0
    int msgctl(int msqid, int cmd, struct msqid_ds *buf);
    msqid：消息队列标识符
    ----------------------------------------------------------------------------
    cmd：消息队列操作
	IPC_STAT：读取消息队列的数据结构msqid_ds，并将其存储在buf指定的结构中
	    内核为每个信号量集合维护着一个msqid_ds个结构
	    struct msqid_ds {
		struct ipc_perm msg_perm;     //所有者和权限
		time_t          msg_stime;    //最后一次向队列发送消息的时间
		time_t          msg_rtime;    //最后一次从队列接收消息的时间
		time_t          msg_ctime;    //队列最后一次改动的时间
		unsigned long   __msg_cbytes; //当前队列所有消息的总长度
		msgqnum_t       msg_qnum;     //当前队列中的消息数量
		msglen_t        msg_qbytes;   //消息队列的最大消息总长度
		pid_t           msg_lspid;    //最后一次给队列发送消息的进程pid
		pid_t           msg_lrpid;    //最后一次从队列接收消息的进程pid
	    }
	--------------------------------------------------------------------
	IPC_SET：将字段msg_perm.uid,msg_perm.gid,msg_perm.mode,msg_qbytes
	从buf指向的结构复制到与这个队列相关的msqid_ds结构中。
	此命令只能由2种进程执行：
	1.一种是其有效用户id等于msg_perm.cuid或msg_perm.uid
	2.另一种是具有超级用户特权的进程。只有超级用户才能增加msg_qbytes的值
	    struct ipc_perm {
		key_t          __key;       /* Key supplied to msgget(2) */
		uid_t          uid;         /* Effective UID of owner */
		gid_t          gid;         /* Effective GID of owner */
		uid_t          cuid;        /* Effective UID of creator */
		gid_t          cgid;        /* Effective GID of creator */
		unsigned short mode;        /* Permissions */
		unsigned short __seq;       /* Sequence number */
	    };
	--------------------------------------------------------------------
	IPC_RMID：从系统中删除该消息队列以及仍在队列中的所有数据，
	这种删除是立即发生的。删除时仍在使用该消息队列的其它进程，
	在他们下次试图对此队列进行操作时，将错误返回EIDRM。
	此命令只能由两种进程执行：
	一种是其有效用户的id等于sem_perm.cuid或sem_perm,uid的进程
	另一种是具有超级用户特权的进程
    ----------------------------------------------------------------------------
    返回值：成功返回0,错误返回-1
#endif
int main(){
//    struct msqid_ds msg1;
    int ret;
    //释放
//    ret=msgctl(65538,IPC_RMID,&msg1);
    ret=msgctl(65538,IPC_RMID,NULL);
    if(ret==-1){
	printf("remove msqid fail\n");
    }
    else
	printf("remove msqid ok\n");
    return 0;
}
