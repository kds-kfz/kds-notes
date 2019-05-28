out()
{
while [ 1 ]
do
    echo "/*****choose 4*****/"
    read -p "Quit?(y/n)" D
    if [ $D == y ];then
    echo "quiting..."
    sleep 2
    exit
    elif [ $D == n ];then
    break
    fi
done
}
