#!/bin/bash
##3.sh
##输出1-100能被3整除到数

for i in `seq 1 100`
do
let a=$i%3
if [ $a -eq 0 ];then
echo -n "$i " 
fi
done
echo



