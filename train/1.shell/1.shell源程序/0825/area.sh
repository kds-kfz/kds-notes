

fun()
{
read -p "long:" a
read -p "wide:" b
}

area()
{
echo "/*****choose 2*****/"
echo "1:delta"
echo "2:oblong"
echo "3:square"
echo "4:quit"

while [ 1 ]
do
read -p "your choose number is:" B
case $B in
1)
fun
let s1=$a*$b/2
echo "delta area:$s1"
;;
2)
fun
let s2=$a*$b
echo "oblong area:$s2"
;;
3)
read -p "long:" d
let s3=$d*$d
echo "square area:$s3"
;;
4)
echo "quit......"
sleep 2
break
;;
*)echo "error!";;
esac
done
}







