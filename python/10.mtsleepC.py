#!/usr/bin/env python3

import threading
import sys
if(sys.version[:1] == "3"):import _thread as thread
else:import thread

from time import sleep, ctime

loops = [4, 2]

def loop(nloop, nsec):
    print('start loop ' + str(nloop) + ' at : ' + ctime())
    sleep(nsec)
    print('loop ' + str(nloop) + ' done at : ' + ctime())

def main():
    print('starting at : ' + ctime())
    threads = []
    nloops = range(len(loops))

    for i in nloops:
        t = threading.Thread(target = loop, args = (i, loops[i]))
        threads.append(t)

    for i in nloops:
        threads[i].start()

    for i in nloops:
        threads[i].join()

    print('all DONE at : ' + ctime())

if __name__=='__main__':
    main()
