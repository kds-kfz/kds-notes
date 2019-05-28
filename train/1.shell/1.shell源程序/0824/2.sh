#!/bin/bash

sum=1
for i in `seq 1 5`
do
let sum=$sum*$i
done
echo "$sum"

sum2=0
for j in `seq 1 100`
do
let sum2=$sum2+$j
done
echo "$sum2"

sum3=0
for k in `seq 100`
do
let sum3=$sum3+$k
done
echo "$sum3"

sum4=0
for l in `seq 1 2 10`
do
echo -n "$l "
done
echo

sum5=0
for l in `seq 1 10`
do
let sum5=$sum5+$l
done
echo "$sum5"



