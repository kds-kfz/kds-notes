#!/bin/bash

clear
echo "输入一个5位数并求出各个位的值"
read -p "请输入一个5位数：" -n 5 A
echo
echo "****************************************"
n1=`expr $A / 10000`
n2=`expr $A / 1000 % 10`
n3=`expr $A / 100 % 10`
n4=`expr $A / 10 % 10`
n5=`expr $A % 10`

echo "万位：$n1"
echo "千位：$n2"
echo "百位：$n3"
echo "十位：$n4"
echo "个位：$n5"
