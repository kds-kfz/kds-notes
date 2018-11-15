#!/usr/bin/env python3

import sys
if(sys.version[:1] == "3"):import _thread as thread
else:import thread

from atexit import register
from random import randrange
from threading import Thread, BoundedSemaphore, Lock
from time import sleep, ctime

lock = Lock()
MAX = 5
candytray = BoundedSemaphore(MAX)

def refill():
    lock.acquire()
    print('Refiling candy ...')
    try:
        candytray.release()
    except ValueError:
        print('full, skipping')
    else:
        print('OK')
    lock.release()

def buy():
    lock.acquire()
    print('Buying candy ...')
    if candytray.acquire(False):
        print('OK')
    else:
        print('empty, skipping')
    lock.release()

def producer(loops):
    for i in range(loops):
        refill()
        sleep(randrange(3))

def consumer(loops):
    for i in range(loops):
        buy()
        sleep(randrange(3))

def _main():
    print('starting at : ' + ctime())
    nloops = randrange(2, 6)
    print('THE CANDY MACHINE (full with %d bars)!' % MAX)
    Thread(target = consumer, args = (randrange(nloops, nloops + MAX +2),)).start()
    Thread(target = producer, args = (nloops,)).start()

def _atexit():
    print('all BONE at : ' + ctime())

if __name__=='__main__':
    _main()


