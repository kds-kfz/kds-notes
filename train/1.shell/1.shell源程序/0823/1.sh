#!/bin/bash
#1.sh
#read -p "please input :" A
#case $A in
#[A-Z]) echo "is big";;
#[a-z]) echo "is small";;
#[0-9]) echo "is number";;
#*) echo "error"
#esac

#echo"火车票订票界面，1,登录 2,注册 3,订票 4,退票 5，退出"
#echo" 重写春夏秋冬"
#echo"输入数字和运算符 计算值 4 + 4 回车出结果"

#echo "1:lodin"
#echo "2:zhuce"
#echo "3:dingpiao"
#echo "4:tuipiao"
#
#read -p "please input a number:" A
#case $A in
#1) echo "login success";;
#2) echo "zhuce";;
#3) echo "dingpiao";;
#4) echo "quit";;
#*) echo "input error";;
#esac

#read -p "please input a number:" A
#B=$((A%10))
#C=$((A/10))
#if [ $B -eq 0 ];then A=0
#elif [ $C -eq 1 -a $B -eq 1 ];then A=a
#elif [ $C -eq 1 -a $B -eq 2 ];then A=b
#fi
#case $A in
#3|4|5) echo "spring";;
#6|7|8) echo "summer";;
#9|0|a) echo "autumn";;
#9|10|11) echo "autumn";;
#1|2|b) echo "winter";;
#12|[1-2]) echo "winter";;
#*) echo "input error";;
#esac

read -p "please input two number:" A B C
case $B in
+) let a=$A+$C 
    echo "$A$B$C=$a";;
-)b=`expr $A - $C` 
    echo "$A$B$C=$b";;
\*)c=$((A*C)) 
    echo "$A$B$C=$c";;
/)d=$[ $A/$C ] 
    echo "$A$B$C=$d";;
*)echo "input error";;
esac










