#!/bin/bash
##6.sh
##输入一个数组，求最大值和最小值，和对应下标
read -p "please input array bit:" N
let n=N-1
for i in `seq 0 $n`
do
read arr[$i]
done
echo

let max=${arr[0]}
let min=${arr[0]}
for k in `seq 0 $n`
do
    if [ ${arr[$k]} -lt $max ];then
    let max=${arr[$k]}
    let a=$k
    fi
    if [ $min -gt ${arr[$k]} ];then
    let min=${arr[$k]}
    let b=$k
    fi
done

echo "your array is:"
for j in ${arr[*]}
do
echo -n "$j "
done

echo

echo "max[$a]:$max"
echo "min[$b]:$min"

if [ $? -eq 0 ];then
echo "/******this program is correct!******/"
fi


