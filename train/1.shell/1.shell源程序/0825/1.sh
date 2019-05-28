#!/bin/bash
##1.sh
##输入数组，求累加，累乘，平均值
read -p "please input array bit:" A
let A--
for i in `seq 0 $A`
do
read arr[$i]
done
for k in ${arr[*]}
do
echo -n "$k "
done
echo
sum=0
amass=1
average=0
for j in `seq 0 $A`
do
let sum=$sum+${arr[$j]}
let amass=$amass*${arr[$j]}
done
let average=$sum/$A
echo "sum=$sum,amass=$amass,average=$average"
echo "/*********************************/"
##5个学生成绩，小于60,则加5并输出
read -p "please input array bit:" A
let A--

for i in `seq 0 $A`
do
read arr[$i]
done

for k in ${arr[*]}
do
echo -n "$k "
done
echo
for j in `seq 0 $A`
do
if [ ${arr[$j]} -lt 60 ];then
let sum=${arr[$j]}+5
let n=$j
echo "arr[$n]=$sum"
fi
done









