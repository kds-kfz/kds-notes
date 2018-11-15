#!/usr/bin/env python3

from socket import *
#-*- coding:utf-8 -*-

HOST = 'localhost'
BUFSIZE = 1024
PORT = int(input('输入端口: '))
ADDR = (HOST, PORT)

while True:
    tcpCliSock = socket(AF_INET, SOCK_STREAM)
    tcpCliSock.connect(ADDR)
    data = input('> ')
    if not data:
        break
    tcpCliSock.send(('%s\r\n' % data).encode())
    data = tcpCliSock.recv(BUFSIZE).decode()
    if not data:
        break
    print(data.strip())
    tcpCliSock.close()
