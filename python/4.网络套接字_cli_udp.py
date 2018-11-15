#!/usr/bin/env python3

from socket import *
from time import ctime

HOST = 'localhost'
PORT = int(input('输入端口: '))
BUFSIZE = 1024
ADDR = (HOST, PORT)


udpCliSock = socket(AF_INET, SOCK_DGRAM)
while True:
    data = input('> ')
    if not data:
        break
    udpCliSock.sendto(data.encode(), ADDR)
    data, ADDR = udpCliSock.recvfrom(BUFSIZE)
    if not data:
        break
    print(data.decode('utf-8'))
    #print('...received from and returned to: ', data.decode('utf-8'))

udpCliSock.close()
