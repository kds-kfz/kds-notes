#!/usr/bin/env python3
#-*- coding:utf-8 -*-

from socketserver import (TCPServer as TCP, StreamRequestHandler as SRH)
from time import ctime

HOST = ''
PORT = int(input('输入端口: '))
ADDR = (HOST, PORT)

class MyRequestHandler(SRH):
    def handle(self):
        print('...connected from: ',self.client_address)
        self.wfile.write(('[%s] %s' %(ctime(), self.rfile.readline().decode())).encode())

tcpServ = TCP(ADDR, MyRequestHandler)
print('watting for connection...')
tcpServ.serve_forever()
