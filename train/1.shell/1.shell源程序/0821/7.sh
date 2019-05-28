#!/bin/bash
a=20
b=20
#if test $a -eq $b
#then
#    echo "a=b"
#fi
#if test $a -ne $b
#then
#    echo "a!=b"
#fi
test $a -ne $b
if [ $? -eq 0 ]
then
    echo "a!=b"
fi

