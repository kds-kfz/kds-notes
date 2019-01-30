//只有 package 名称为 main 的包可以包含 main 函数
package main

//通过 import 关键字来导入其它非 main 包
import (
    //使用别名包
    my "fmt"
)
 
//单行注释
/*多行注释*/
//原生字符串：`\n\.abc\t\` == "\\n\\.abc\\t\\"

/*
关键字
const：常量
var：全局变量
type：对struct或interface声明
func：对函数声明
*/
//调用格式：<PackageName>.<FuncName>

//函数名首字母 小写 即为private，函数名首字母 大写 即为public

//常量
//等号右侧必须是常量或者常量表达式
//常量的值在编译时就已经确定 
//常量表达式中的函数必须是内置函数
const letter = 'A'
const (
    //3个字符表示一个文字
    text = "你好，二〇壹玖年壹月贰拾捌！"
    length = len(text) //42 = 14 *3
    num = letter * 5
    //定义常量组时 常量计数器 从0开始计数 每遇一个const则重置
    n_1 = iota
)
const (
    a2 = 'A'
    //b2的值也是A
    b2
    c2 = iota
    //d2的值是3
    d2
)
const (
    E2 = iota
    //f2的值是1
    F2
)
const (
    //星期枚举 第一个常量不可省略表达式
    Mon = iota
    Tue
    Wed
    Thu
    Fri
    Sta
    Sun
)
const (
    // 利用<<实现计算机存储单位枚举
    _  = iota
    KB float64 = 1 << (iota * 10)
    MB
    GB
    TB
    PB
    EB
    ZB
    YB
)

//全局变量
var (
    //常规方式
    num_1 = "张三"
    //使用并行方式以及类型推断
    num_2, num_3 = 3.14, "3.14"
    //不可以省略var
    //num_4 := 3.14 //错误
)
//一般类型声明 类型别名
type (
    newType int
    type_1 float32
    type_2 string
    type_3 byte
)

//变量声明赋值
var b float32 = 3.14
//省略类型，由系统判断
var c = 3.14

var A1, B1, C1 int = 65, 2, 3

//指针 采用”.”选择符来操作指针目标对象的成员
//操作符”&”取变量地址，使用”*”通过指针间接访问目标对象
//默认值为 nil 而非 NULL
//在Go当中，++ 与 -- 是作为语句而并不是作为表达式

//func函数
func f1(a [5]int) { 
    my.Println("数组遍历")
    for i, n := range a{
        my.Println(i, n)
    }
}
func f2(a *[5]int) { 
    my.Println("切片遍历")
    for i, n := range a{
        my.Println(i, n)
    }
}
func f3(a *[5]string){ 
    for i:=0; i < len(a); i++ {
        my.Printf("Person at %d is %s\n", i, a[i])
    }
}
func f4(a *[3]float64) (sum float64) {
    //复制一份v遍历 不会改变原数组中的值
    for _, v := range a {
        sum += v
    }
    return sum
    //等价于 return
}
func f5(a map[string]int){
    my.Println("map遍历")
    for k, v := range a{
        my.Println(k, v)
    }
}

func main() {
    //多个变量声明，赋值 只能写在函数里
    var A, B, C int
    A, B, C = 1, 2, 3
    //变量声明
    var a bool
    //变量赋值 函数内赋值
    a = true
    //变量声明赋值简写 函数内
    d := 3.14
    my.Println("Hello, 世界")
    my.Println(a,d,A,B,C)
    //类型转换
    my.Println(int(c))
    //bool 不能转换
    //var D bool = false
    //D := int(D)
    //表示将数据转换成文本格式
    a1 := string(A1)
    my.Println(a1)

    //常量
    my.Println(num,text,length) //325 = 65 * 5
    my.Println(n_1)
 
    //if语句
    if a, b, c := 1, 2, 3; a + b + c >6 {//左大括号必须和条件语句或else在同一行
        my.Println("大于6")
    }else{ //左大括号必须和条件语句或else在同一行
        my.Println("小于等于6")
        my.Println(a)//1
    }
    my.Println(a)//true

    //for循环
    b = 1
    for{
        b ++
        if b >3{
            break
        }
    }
    my.Println("for循环 方式1:",b)

    b = 1
    for b<=3{
        b ++
    }
    my.Println("for循环 方式2:",b)

    b = 1
    for a := 0; a<3; a++ {
        b ++
    }
    my.Println("for循环 方式3:",b)

    //switch选择语句
    //可以使用任何类型或表达式作为条件语句
    //不需要写break，一旦条件符合自动终止
    //如希望继续执行下一个case，需使用fallthrough语句
    //支持一个初始化表达式（可以是并行方式），右侧需跟分号
    //左大括号必须和条件语句在同一行
    b = 1
    switch b{
        case 0:
            my.Println("switch语句 方式1: b = 0")
        case 1:
            my.Println("switch语句 方式1: b = 1")
    }
    my.Println("switch语句 方式1:",b)
    switch{
        case b >= 0:
            my.Println("switch语句 方式2: b >= 0")
        case b > 1:
            my.Println("switch语句 方式2: b > 1")
    }
    my.Println("switch语句 方式2:",b)
    switch b := 2; {
        case b >= 0:
            my.Println("switch语句 方式3: b >= 0")
            fallthrough
        case b > 1:
            my.Println("switch语句 方式3: b > 1")
    }
    my.Println("switch语句 方式3:",b)

    //goto break continue 跳转语句
    //三个语法都可以配合标签使用
    //标签名区分大小写，若不使用会造成编译错误
    //Break与continue配合标签可用于多层循环的跳出
    //Goto是调整执行位置，与其它2个语句配合标签的结果并不相同
    LABEL1:
        for{
            for a:=0; a<10; a++{
                if a>2{
                    //跳出外层for循环
                    break LABEL1
                }else{
                    my.Println("break语句:",a)
                }
            }
        }
    LABEL2:
        for a:=0; a<10; a++{
            for{
                my.Println("continue语句:",a)
                //每次重置a的值，导致外层死循环
                //goto LABEL2 
                //a的值没有被重新初始化
                continue LABEL2
            }
        }

    //array 数组
    //定义数组的格式：var <varName> [n]<type>，n>=0
    //数组之间可以使用==或!=进行比较，但不可以使用<或>
    //注意区分指向数组的指针和指针数组
    //go语言也有多维数组
    //... 可同样可以忽略，从技术上说它们其实变化成了切片
    w := [...]string{"a", "b", "c", "d", "e"}
    for i:= range w{
        my.Println("w array",i,"is",w[i])
    }
    f3(&w)

    //arr1 的类型是 *[5]int
    var arr1 = new([5]int)
    arr2 := *arr1
    arr2[2] = 100
    //arr2赋值不会影响arr1里的值
    my.Println("arr2[2]:",arr2[2],"arr1[2]",arr1[2])
    var arr3 = [5]int{1, 2, 3, 4, 5}
    //只有索引 3 和 4 被赋予实际的值 这里的数组长度可以写成... 忽略
    var arr4 = [5]string{3: "Chris", 4: "Ron"}
    arr5 := [3]float64{7.0, 8.5, 9.1}
    arr6 := map[string]int{"hello":1, "world":2}
    
    f1(arr3)
    f2(&arr3)
    f3(&arr4)
    f5(arr6)
    x := f4(&arr5)
    my.Printf("The sum of the array is: %f\n", x)
}

