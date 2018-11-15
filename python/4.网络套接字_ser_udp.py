#!/usr/bin/env python3

from socket import *
from time import ctime

HOST = ''
PORT = int(input('输入端口: '))
BUFSIZE = 1024
ADDR = (HOST, PORT)

udpSerSock = socket(AF_INET, SOCK_DGRAM)
udpSerSock.bind(ADDR)

while True:
    print('watting for message...')
    data, addr = udpSerSock.recvfrom(BUFSIZE)
    #udpSerSock.sendto(('[%s] %s' %(ctime(),data)).encode('utf-8'), addr)
    udpSerSock.sendto(('[%s] %s' %(ctime(),data.decode('utf-8'))).encode(), addr)
    print('...received from and returned to: ',addr)
udpSerSock.close()
