#!/bin/bash

clear
echo "四则运算"
read -p "参数1：" A
read -p "参数2：" B
echo
echo "method:1 expr 表达式 "
n1=`expr $A + $B`
n2=`expr $A - $B`
n3=`expr $A \* $B`
n4=`expr $A / $B`
n5=`expr $A % $B`

echo "$n1 = $A + $B"
echo "$n2 = $A - $B"
echo "$n3 = $A * $B"
echo "$n4 = $A / $B"
echo "$n5 = $A % $B"
echo

echo "method:2 let 表达式 "
let n1=$A+$B
let n2=$A-$B
let n3=$A*$B
let n4=$A/$B
let n5=$A%$B

echo "$n1 = $A + $B"
echo "$n2 = $A - $B"
echo "$n3 = $A * $B"
echo "$n4 = $A / $B"
echo "$n5 = $A % $B"
echo

echo "method:3 \$[] 表达式 "
n1=$[ $A+$B ] 
n2=$[ $A-$B ]
n3=$[ $A*$B ]
n4=$[ $A/$B ]
n5=$[ $A%$B ]

echo "$n1 = $A + $B"
echo "$n2 = $A - $B"
echo "$n3 = $A * $B"
echo "$n4 = $A / $B"
echo "$n5 = $A % $B"
echo

echo "method:4 \$(()) 表达式 "
n1=$(( $A+$B ))
n2=$(( $A-$B ))
n3=$(( $A*$B ))
n4=$(( $A/$B ))
n5=$(( $A%$B ))

echo "$n1 = $A + $B"
echo "$n2 = $A - $B"
echo "$n3 = $A * $B"
echo "$n4 = $A / $B"
echo "$n5 = $A % $B"
echo
