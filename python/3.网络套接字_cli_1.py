#!/usr/bin/env python3

from socket import *

HOST = 'localhost'
#PORT = 8989
BUFSIZE = 1024
PORT = int(input('输入端口: '))

ADDR = (HOST, PORT)
tcpCliSock = socket(AF_INET, SOCK_STREAM)
tcpCliSock.connect(ADDR)

while True:
    data = input('> ')
    if not data:
        break
    #encode编码，python3默认是str 
    tcpCliSock.send(data.encode())
    #encode解码
    data = tcpCliSock.recv(BUFSIZE).decode('utf-8')
    if not data:
        break
    print(data)

tcpCliSock.close()
