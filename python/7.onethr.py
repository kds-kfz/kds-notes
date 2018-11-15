#!/usr/bin/env python3

from time import sleep, ctime

def loop():
    print('start loop 0 at : %s' % ctime())
    sleep(4)
    print('loop 0 done at : %s' % ctime())


def loop1():
    print('start loop 1 at : %s' % ctime())
    sleep(2)
    print('loop 1 done at : %s' % ctime())

def main():
    print('starting at : %s' % ctime())
    loop()
    loop1()
    print('all DONE at : %s' % ctime())

if __name__=='__main__':
    main()
