#!/bin/bash                                                                                     
DESTDIR=.
HOST='127.0.0.1'
PORT='7000'
PROCESSNAME='test.cgi'
sudo killall -9 $PROCESSNAME
echo "=====killed====="
RESTART="sudo /usr/local/bin/spawn-fcgi -a $HOST -p $PORT -f $DESTDIR/$PROCESSNAME -F 1"
echo $RESTART
$RESTART
echo "======done======"
ps -ef | grep $PROCESSNAME
