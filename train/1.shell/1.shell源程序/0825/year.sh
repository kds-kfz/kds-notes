

ping=(31 28 31 30 31 30 31 31 30 31 30 31)

run=(31 29 31 30 31 30 31 31 30 31 30 31)

daycount=0

count()
{

echo "/*****choose 3*****/"

read -p "year:" q
read -p "month:" w
read -p "day:" e

let Q1=$q%4
let Q2=$q%100
let Q3=$q%100

daycount=0


if [ $Q1 -eq 0 -a $Q2 -ne 0 -o $Q3 -eq 0 ];then
    echo "$q is leap year"
    if [ $w -gt 1 ];then
    let end=$w-2
    for t in `seq 0 $end`
    do
	let daycount=$daycount+${ping[$t]}
    done
	let daycount=$daycount+$e
    echo "it is $daycount"
    fi
else
    echo "$q is common year"
    if [ $w -gt 1 ];then
    let end1=$w-2
    for t1 in `seq 0 $end1`
    do
	let daycount=$daycount+${run[$t1]}
    done
	let daycount=$daycount+$e
    echo "it is $daycount"
    fi
    if [ $w -eq 1 ];then
    echo "it is $e"
    fi

fi


}

