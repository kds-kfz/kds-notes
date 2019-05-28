#!/bin/bash
clear
echo "练习：输入一个5位数，分别输出每一位到值"

read -p "请输入5位数：" -n 5 A
echo

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

read -p "请输入5位数：" -n 5 A
echo

let n1=$A/10000
let n2=$A/1000%10
let n3=$A/100%10
let n4=$A/10%10
let n5=$A%10

echo "万位：$n1"
echo "千位：$n2"
echo "百位：$n3"
echo "十位：$n4"
echo "个位：$n5"

read -p "请输入5位数：" -n 5 A
echo

n1=$[ $A/10000 ]
n2=$[ $A/1000%10 ]
n3=$[ $A/100%10 ]
n4=$[ $A/10%10 ]
n5=$[ $A%10 ]

echo "万位：$n1"
echo "千位：$n2"
echo "百位：$n3"
echo "十位：$n4"
echo "个位：$n5"

read -p "请输入5位数：" -n 5 A
echo

n1=$(($A/10000))
n2=$(($A/1000%10))
n3=$(($A/100%10))
n4=$(($A/10%10))
n5=$(($A%10))

echo "万位：$n1"
echo "千位：$n2"
echo "百位：$n3"
echo "十位：$n4"
echo "个位：$n5"

echo "end"


