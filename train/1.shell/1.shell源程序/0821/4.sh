#!/bin/bash
clear
#四则运算

read -p "参数1:" n1
read -p "参数2:" n2
#方法1：
#n3=`expr $n1 + $n2`
#n4=`expr $n1 - $n2`
#n5=`expr $n1 \* $n2`
#n6=`expr $n1 / $n2`
#n7=`expr $n1 % $n2`
#
#echo "$n3=$n1+$n2"
#echo "$n4=$n1-$n2"
#echo "$n5=$n1*$n2"
#echo "$n6=$n1/$n2"
#echo "$n7=$n1%$n2"
#方法2：
#let n3=$n1+$n2
#let n4=$n1-$n2
#let n5=$n1*$n2
#let n6=$n1/$n2
#let n7=$n1%$n2
#
#echo "$n3=$n1+$n2"
#echo "$n4=$n1-$n2"
#echo "$n5=$n1*$n2"
#echo "$n6=$n1/$n2"
#echo "$n7=$n1%$n2"
#方法3：
#n3=$[ $n1+$n2 ]
#n4=$[ $n1-$n2 ]
#n5=$[ $n1*$n2 ]
#n6=$[ $n1/$n2 ]
#n7=$[ $n1%$n2 ]
#
#echo "$n3=$n1+$n2"
#echo "$n4=$n1-$n2"
#echo "$n5=$n1*$n2"
#echo "$n6=$n1/$n2"
#echo "$n7=$n1%$n2"
#方法4：
n3=$((n1+n2))
n4=$((n1-n2))
n5=$((n1*n2))
n6=$((n1/n2))
n7=$((n1%n2))

echo "$n3=$n1+$n2"
echo "$n4=$n1-$n2"
echo "$n5=$n1*$n2"
echo "$n6=$n1/$n2"
echo "$n7=$n1%$n2"




