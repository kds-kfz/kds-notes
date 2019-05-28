#include <stdio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
/*学生信息的结构*/
typedef struct stu
{
	int num;
	char name[20];
}STU_T;
/*登录信息的结构*/
typedef struct myregister
{
	char account[20];
	char passwd[20];

}REGISTER_T;
/*消息类型(可以用1,2,3等分别代表登录和注册等)以及长度*/
typedef struct msghead
{
	int len;
	int msgtype;// 1:register 3:login 
}MSGHEAD_T;
/*操作的菜单栏*/
void menu1()
{
	printf("1.register.\n");
	printf("2.login.\n");
	printf("please enter your choose:\n");
}

void myregister(int fd)
{
	/*
	 当注册信息的时候,需要发送给服务器，消息的类型(比方说1代表注册)
	 [1][len][data]--->数据格式
	 */
	REGISTER_T myre;
	char buff[1024] = {0};
    MSGHEAD_T msghead;

	memset(&myre, 0, sizeof(myre));
	printf("please input your account:\n");
	scanf("%s", myre.account);
	printf("please input your passwd:\n");
	scanf("%s", myre.passwd);

	int len;
	len = sizeof(msghead) + sizeof(myre);
	msghead.len = len;
	msghead.msgtype = 1;

	msghead.len = htonl(msghead.len);
	msghead.msgtype = htonl(msghead.msgtype);

	memcpy(buff, &msghead, sizeof(msghead));
	memcpy(buff + sizeof(msghead), &myre, sizeof(myre));//完成发送数据的组装
    
	int ret;
	ret = write(fd, buff, len);
	if(ret < 0)
	{}

}
void login(int fd)
{
	REGISTER_T myre;
    char buff[1024] = {0};
	MSGHEAD_T msghead;
	
	//short int

	memset(&myre, 0, sizeof(myre));
    printf("please input your account:\n");
	scanf("%s", myre.account);
	printf("please input your passwd:\n");
	scanf("%s", myre.passwd);
    
	int len;
	len = sizeof(msghead) + sizeof(myre);
	msghead.len = len;
	msghead.msgtype = 3;

	msghead.len = htonl(msghead.len);
	msghead.msgtype = htonl(msghead.msgtype);//32  (htonl htons :) 16 (ntohl  ntohs)
    
	memcpy(buff, &msghead, sizeof(msghead));
	memcpy(buff + sizeof(msghead), &myre, sizeof(myre));

	int ret;

	ret = write(fd, buff, len);
	if(ret < 0)
	{}

	return;

}

void * handler(void * arg)
{
	int *p = (int *)arg;
	int fd;

	fd = *p;
    char buff[1024];
	int ret;
	STU_T stu;
	int temp;
	while(1)
	{
		menu1();
		scanf("%d", &temp);
		if(temp == 1)
		{
			myregister(fd);

		}
		else if(temp == 2)
		{
			login(fd);
		}

	}

	return NULL;
}

int main(int argc, char ** argv)
{
	int fd;
	int ret;
	pthread_t pid;
	REGISTER_T myre;
	struct sockaddr_in serverAddr;

	if(argc != 3)
	{
		printf("para error\n");
		return -1;
	}

	fd = socket(AF_INET, SOCK_STREAM, 0);
	if(fd < 0)
	{}
    
	memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = inet_addr(argv[1]);
	serverAddr.sin_port = htons(atoi(argv[2]));

	ret = connect(fd, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
	if(ret < 0)
	{
		perror("connect");
		return -1;
	}

	ret = pthread_create(&pid, NULL, handler, &fd);
	if(ret < 0)
	{}
	
	char buff[1024];
	MSGHEAD_T msghead;
	int result;
	while(1)
	{
		memset(buff, 0, 1024);
		ret = read(fd, buff, 1024);
        if(ret < 0)
		{
			perror("read");
			return -1;
		}
		else if(ret == 0)
		{
			//close(fd);
			break;
		}
		else
		{//msghead + result
			memcpy(&msghead, buff, sizeof(msghead));
			msghead.len = ntohl(msghead.len);
			msghead.msgtype = ntohl(msghead.msgtype);
			if(msghead.msgtype == 2)//注册的返回消息  //0表示成功  1表示失败
			{
				memcpy(&result, buff + sizeof(msghead), 4);
				result = ntohl(result);
				if(result == 0)
				{
					printf("注册成功\n");
				}
				else if(result == 1)
				{
					printf("注册失败\n");
				}
			}
			else if(msghead.msgtype == 4)
			{
				memcpy(&result, buff + sizeof(msghead), 4);
				result = ntohl(result);
				if(result == 0)
				{
					printf("登录成功\n");
					memcpy(&myre,buff+sizeof(msghead)+4,sizeof(myre));
					printf("%s,%s\n",myre.account,myre.passwd);
				}
				else if(result == 1)
				{
					printf("用户名输入有误\n");
				}
				else if(result == 2)
				{
					printf("密码输入有误\n");
				}
			}
		}
	}

	pthread_join(pid, NULL);

	return 0;

}

