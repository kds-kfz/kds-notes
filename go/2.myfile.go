package main

import (
    "fmt"
    "io/ioutil"
    "strings"
)

func main(){
    Ioutil("kd-fs-done.businessid")
}

//文件操作
//func ReadAll(r io.Reader) ([]byte, error)
//func ReadDir(dirname string) ([]os.FileInfo, error)
//func ReadFile(filename string) ([]byte, error)
//ReadAll 函数被定义为从源中读取数据直到EOF，它是不会去从返回数据中去判断EOF来作为读取成功的依据
//ReadDir 读取一个目录，并返回一个当前目录下的文件对象列表和错误信息
//ReadFile 读取文件内容，并返回[]byte数据和错误信息。err == nil时，读取成功

func Ioutil(filename string){
    if contents, err := ioutil.ReadFile(filename); err == nil{
        result := strings.Replace(string(contents), "\n", "", 1)
        fmt.Println(result)
    }
}
