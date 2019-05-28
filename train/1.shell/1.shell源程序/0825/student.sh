

mark()
{

echo "/*****choose 1*****/"
read -p "/*****please input student number:" A
let n=A-1
sum=0
for i in `seq 0 $n`
do
    read arr[$i]
done

for j in ${arr[*]}
do
let sum=$sum+$j
done
echo ${arr[*]}
let ave=$sum/$A
echo "ave=$ave"
}
