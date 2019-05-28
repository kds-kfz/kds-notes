#!/bin/bash
#echo "判断一个文件是否是普通文件，如果是，把它拷贝到11文件目录里"
#echo "判断闰年（能被4整除并且不能被100整除，或能被400整除）"
#echo "判断文件是否是可执行，不可执行就加可执行权限"
#echo "/**************第1题***********/"
#if [ $# -eq 1 ];then
#    test -f $1
#    if [ $? -ne 0 ];then
#	echo "it is a file"
#    else
#	echo "it isn't a file"
#    if [ -r $1 ];then
#	echo "is read"
#    elif [ -w $1 ];then
#	echo "is write"
#    elif [ -x $1 ];then
#	echo "is action"
#    fi
#    if [ -e 11 ];then
#	echo "11 file is in"
#	mv $1 11
#	echo "operate success"
#    else
#	mkdir 11
#	echo "11 file is set up"
#	echo "operate success"
#	mv $1 11
#    fi
#    fi
#fi

#echo "/**************第2题***********/"
#read -p "please input year:" A
#B=`expr $A % 4`
#C=`expr $A % 100`
#D=`expr $A % 400`

#if [ $B -eq 0 -a $C -ne 0 -o $D -eq 0 ];then
#    echo "is leap year"
#else
#    echo "is common nian"
#fi

#if [ `expr $A % 4` -eq 0 -a `expr $A % 100` -ne 0 -o `expr $A % 400` -eq 0 ];then
#    echo "is leap year"
#else
#    echo "is common year"
#fi

#echo "/**************第3题***********/"

if [ $# -eq 1 ];then
    if [ -e $1 ];then
	echo "the file is in"
	ls -l
	if [ -x $1 ];then
	    echo "file is action"
	else
	    chmod u+x $1
	    ls -l
	fi
    else
	echo "file isn't in"
    fi
else
    echo "error"
fi


