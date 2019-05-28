#!/bin/bash
#clear
#echo "/****与：-a***或：-o***非：!***************************/"
#read -p "zhanghao:" A
#read -p "mima:" B
#if [ $A == aplle -a $B -eq 123 ];then
#    echo "ok"
#fi

#echo " 输入成绩90-100优，80-89良，70-79中，70以下不及格"

#echo "/***********method1************/"
#echo "please input student's mark"
#read -p "student's mark:" A
#if [ $A -ge 0 -a $A -le 100  ];then
#    if [ $A -ge 90 -a $A -le 100  ];then
#    echo "test mark:A"
#    fi
#    if [ $A -ge 80 -a $A -le 89  ];then
#    echo "test mark:B"
#    fi
#    if [ $A -ge 70 -a $A -le 79  ];then
#    echo "test mark:C"
#    fi
#    if [ $A -ge 0 -a $A -le 69  ];then
#    echo "test mark:D"
#    fi
#else
#    echo "input error!"
#fi

#echo "/***********method2************/"
#echo "please input student's mark"
#read -p "student's mark:" A
#if [ $A -lt 0 -o $A -gt 100 ];then
#    echo "input error"
#else
#    if [ $A -ge 90 ];then
#    echo "student mark:A"
#    else if [ $A -ge 80 ];then
#	echo "student mark:B"
#	else if [ $A -ge 70 ];then
#	    echo "student mark:C"
#	    else
#		echo "student mark:D"
#	    fi
#	fi
#    fi
#fi

#echo "/***********method3************/"
#echo "please input student's mark"
#read -p "student's mark:" A
#if [ $A -lt 0 -o $A -gt 100 ];then
#    echo "input error"
#elif [ $A -ge 90 ];then
#    echo "student mark:A"
#elif [ $A -ge 80 ];then
#    echo "student mark:B"
#elif [ $A -ge 70 ];then
#    echo "student mark:C"
#else
#    echo "student mark:D"
#fi

echo "输入数字判断春夏秋冬（3-5春，6-8夏，9-11秋，12-2冬）"
echo "please input number"
read -p "input month number is:" A
#if [ $A -lt 1 -o $A -gt 12 ];then
#    echo "input error"
#elif [ $A -ge 3 -a $A -le 5 ];then
#    echo "spring"
#elif [ $A -ge 6 -a $A -le 8 ];then
#    echo "summer"
#elif [ $A -ge 9 -a $A -le 11 ];then
#    echo "autumn"
#else
#    echo "winter"
#fi

if [ $A -lt 1 -o $A -gt 12 ];then
    echo "input error"
elif [ $A -eq 12 -o $A -le 2 -a $A -gt 0  ];then
    echo "winter"
elif [ $A -gt 8 ];then
    echo "autumn"
elif [ $A -ge 6 ];then
    echo "summer"
else
    echo "spring"
fi













if [ $? -eq 0 ];then
    echo "/**********Program correct**********/"
fi


