#!/bin/bash



arr=( a 0 A -6 b 15 B 68 C 48 c -78 )

for i in `seq 0 11`
do
echo  "${arr[$i]} " 
done | sort -n -r

#done | sort 
#done | sort -r
#done | sort -n 
#done | sort -n -r



