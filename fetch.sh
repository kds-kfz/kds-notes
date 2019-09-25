#!/bin/bash
unset SSH_ASKPASS
git fetch
git rebase origin/develop
if [ $? -eq 0 ];then
echo "/~~~~~ oper success! ~~~~~/"
fi
exit
