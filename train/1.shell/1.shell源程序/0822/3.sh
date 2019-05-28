#!/bin/bash
#echo "判断文件是否可执行，不可执行就加可执行权限"
if [ $# -eq 1 ];then
    if [ -e $1 ];then
	if [ -d $1 ];then
	    echo "mulu"
	fi
	if [ -f $1 ];then
	    echo "wenjian"
	fi
	if [ -r $1 ];then
	    echo "read"
	fi
	if [ -w $1 ];then
	    echo "write"
	fi
	if [ -x $1 ];then
	    echo "zhixing"
	fi
    else
	touch $1
	chmod u+x $1
    fi
else
    echo "error!"
fi

