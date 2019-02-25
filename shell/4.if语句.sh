#!/bin/bash

clear
A=20
B=22

#或者：-o
#同时：-a 
#大于等于：-ge
#小于等于：-le
#等于：-eq 
#小于：-lt 
#大于：-gt

echo "method:1 test表达式"
if test $A -eq $B
then
    echo "$A = $B"
fi
if test $A -ne $B 
then
    echo "$A != $B"
fi

echo
echo "method:2 []表达式"
if [ $A -eq $B ]; then
    echo "$A = $B"
fi
if [ $A -ne $B ]; then
    echo "$A != $B"
fi

echo
echo "method:3 if else表达式"
if [ $A -eq $B ]; then
    echo "$A = $B"
    else if [ $A -ne $B ]; then
        echo "$A != $B"
        fi
fi
echo
echo "method:4 elif表达式"
if [ $A -eq $B ]; then
    echo "$A = $B"
elif [ $A -ne $B ]; then
    echo "$A != $B"
fi

echo "检查以上语句是否正确"

