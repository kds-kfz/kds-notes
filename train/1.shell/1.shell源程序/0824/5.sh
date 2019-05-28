#!/bin/bash
##5.sh
##定义一个数组
arr=(1 2 3 4 5 6 7 a b c )
count=${#arr[*]}
echo $count

##改变数组arr[0]的值
arr[0]=99

##改变数组arr[0]的值
echo ${arr[0]}

##循环输入一个5位数组
for i in `seq 0 4`
do
    read arr[$i]
done
echo ${arr[*]}

##循环输出数组arr
for j in ${arr[*]}
do
echo -n "$j "
done
echo











