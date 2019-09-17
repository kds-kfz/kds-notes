#!/bin/bash
unset SSH_ASKPASS
read -p "是否进行代码强推[y/n]" A
if [ $A == y ]; then
    git push origin develop -f
elif [ $A == n ]; then
    git push origin develop
fi

if [ $? -eq 0 ];then
echo "/~~~~~ oper success! ~~~~~/"
fi
exit
