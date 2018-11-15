#!/usr/bin/env python3

#python
#import thread
#python3
#import _thread

#兼容python2和python3的写法：
import sys
if(sys.version[:1] == "3"):import _thread as thread
else:import thread

from time import sleep, ctime

def loop0():
    print('start loop 0 at : ' + ctime())
    sleep(4)
    print('loop 0 done at : ' + ctime())

def loop1():
    print('start loop 1 at : ' + ctime())
    sleep(2)
    print('loop 1 done at : ' + ctime())

def main():
    print('starting at : ' + ctime())
    thread.start_new_thread(loop0, ())
    thread.start_new_thread(loop1, ())
    sleep(6)
    print('all DONE at : ' + ctime())

if __name__=='__main__':
    main()
