#!/usr/bin/env python3
#-*- coding:utf-8 -*-

from twisted.internet import protocol, reactor
from time import ctime

PORT = int(input('输入端口: '))

class TSServProtocol(protocol.Protocol):
    def connectionMade(self):
        clnt = self.clnt = self.transport.getPeer().host
        print('...connected from: ',clnt)
    
    def dataReceived(self, data):
        delf.transport.write(('[%s] %s' %(ctime(), data.encode())).encode())

factory = protocol.Factory()
factory.protocol = TSServProtocol
print('watting for connection...')
reactor.listenTCP(PORT, factory)
reactor.run()
