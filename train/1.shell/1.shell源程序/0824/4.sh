#!/bin/bash
##4.sh
##*.
##*.*.
##*.*.*.
##*.*.*.*.
#read -p "please input row:" A
#for i in `seq 1 $A`
#do
#    for j in `seq 1 $i`
#    do
#	echo -n "*."
#    done
#    echo
#done

##    *
##   ***
##  *****
## *******
##*********

#read -p "please input a number:" B
#c=1
#for i in `seq 1 $B`
#do
#    let B--
#    for j in `seq 1 $B`
#    do
#	echo -n " "
#    done
#    for k in `seq 1 $c`
#    do
#	echo -n "*"
#    done
#    let c=i*2+1
#    echo
#done

##输出99乘法表

a=1
sum=0
for i in `seq 1 9`
do  
    for j in `seq 1 $i`
    do
	let sum=$i*$j
	echo -n "$j*$i=$sum "
    done
    echo
done

























