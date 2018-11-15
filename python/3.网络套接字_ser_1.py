#!/usr/bin/env python3

from socket import *
from time import ctime

HOST = ''
#PORT = 8989
BUFSIZE =1024
PORT = int(input('输入端口: '))
ADDR = (HOST, PORT)

#创建套接字
tcpSerSock = socket(AF_INET, SOCK_STREAM)
#绑定地址和端口
tcpSerSock.bind(ADDR)
#设置监听数
tcpSerSock.listen(5)
while True:
    print('waitting for connection ...')
    tcpCliSock, addr = tcpSerSock.accept()
    print('...connected from: ',addr)

    while True:
        #decode解码
        data = tcpCliSock.recv(BUFSIZE).decode('utf-8')
        if not data:
            break
        tcpCliSock.send(('[%s] %s' %(ctime(),data)).encode())
    tcpCliSock.close()

tcpSerSock.close()
