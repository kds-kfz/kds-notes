#!/bin/bash
#4.sh
#死循环登录，3次错误显示并退出
#count=0
#while [ 1 ]
#do
#read -p "please input ID:" A
#read -p "please input passwd:" B
#if [ $A == apple -a $B == 123 ];then
#    echo "login success"
#    break
#else 
#    let count++
#    if [ $count -le 3 ];then
#    echo "error $count times"
#	if [ $count -eq 3 ];then
#	exit
#        fi
#    fi
#fi
#done

#输出
#*****
#*****
#*****
#*****

#i=0
#j=0
#while [ $i -lt 4 ]
#do
#    j=0
#    while [ $j -lt 5 ]
#    do
#	echo -n "*"
#	let j++
#    done
#    echo
#    let i++
#done

#登录流程练习
echo "1:zhuce"
echo "2:login"
echo "3:quit"
while [ 1 ]
do
read -p "please choose is:" C
case $C in
1) echo "zhuce" ;;
2) echo "logining"
count=0
while [ 1 ]
do
read -p "please input ID:" A
read -p "please input passwd:" B
if [ $A == apple -a $B == 123 ];then
    echo "login success"
#    break
    break 2
else 
    let count++
    if [ $count -le 3 ];then
    echo "error $count times"
	if [ $count -eq 3 ];then
#	break
	exit
        fi
    fi
fi
done
;;
3) exit ;;
*) echo "input error!" ;;
esac
done











