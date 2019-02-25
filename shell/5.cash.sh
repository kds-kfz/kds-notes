#!/bin/bash

clear
read -p "please input a number：" A
case $A in
1) echo "choose 1";;
2) echo "choose 2"
    echo "welcome";;
3|4|5) echo "choose 3 or 4 or 5";;
*) echo "input error";;
esac

if [ $? -eq 0 ];then
    echo "**********project correct**********"
fi
