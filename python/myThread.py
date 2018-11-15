#!/usr/bin/env python3

import threading
import sys
if(sys.version[:1] == "3"):import _thread as thread
else:import thread

from time import ctime


class MyThread(threading.Thread):
    def __init__(self, func, args, name = ''):
        threading.Thread.__init__(self)
        self.name = name
        self.func = func
        self.args = args

    def getResult(self):
        return self.res

    def run(self):
        print('starting ' + self.name + ' at : ' \
                + ctime())
        self.res = self.func(*self.args)
        print(self.name + ' finished at : ' \
                + ctime())

def loop(nloop, nsec):
    print('start loop ' + str(nloop) + ' at : ' + ctime())
    sleep(nsec)
    print('loop ' + str(nloop) + ' done at : ' + ctime())

