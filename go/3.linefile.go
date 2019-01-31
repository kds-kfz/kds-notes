package main

import (
    "fmt"
    "os"
    "io"
    "bufio"
    "strings"
    "path"
    "sync"
    "encoding/json"
)

/*
 example:
 {
    "ptjy/ptyw":{
        "login":"ptjy_ptyw_login",
        "cccx":"ptjy_ptyw_cccx"
    }
}
 */

//全局变量
var (
    input_file = "kd-fs-done.businessid"
    output_file = "business-map.json"
    mutex sync.Mutex
    output = make(map[string]map[string]string)
)


func main(){
    Read_lines_file(input_file)
}

func Read_lines_file(filename string){
    if file, err := os.OpenFile(filename, os.O_RDWR, 0666); err == nil{
        if stat, err := file.Stat(); err == nil{
            var size = stat.Size()
            fmt.Println("file size =", size)
            buf := bufio.NewReader(file)
            for{
                if line, err := buf.ReadString('\n'); err == nil{
                    line = strings.TrimSpace(line)
                    //字符串替换
                    line = strings.Replace(line, "/", "_", -1)
                    //fmt.Println(line)
                    //处理每行内容，生成字典
                    //L := strings.Count(line,"") - 1 //字符总长 最后一个是\0
                    //a := strings.Index(line, "_") //第一次出现的位置
                    b := strings.LastIndex(line, "_") //最后一次出现的位置
                    H := string([]byte(line)[:b])
                    Z := string([]byte(line)[b+1:])
                    if _, ok := output[H]; ok {  
                        output[H][Z] = line
                    }else{
                        output[H] = make(map[string]string)
                    }
                }else{
                    if err == io.EOF{
                        fmt.Println("File read ok!")
                        break
                    }else{
                        fmt.Println("Read file err!",err)
                        return
                    }
                }
            }
        }else{
            panic(err)
        }
    defer file.Close()
    }else{
        fmt.Println("Open file error!",err)
        return
    }

    // 将这个映射序列化到JSON 字符串
    if data, err := json.MarshalIndent(output, "", "    "); err ==nil{
        //fmt.Println(string(data)) //输出到终端
        WriteConfig(output_file, data)
    }else{
        fmt.Println("ERROR:", err)
    }
       
}

func WriteConfig(cfg string, jsonByte []byte) {
    if cfg == "" { fmt.Println("outputfile is empty") }

    if _, err := WriteBytes(cfg,jsonByte); err != nil{
        fmt.Println("ERROR:", err)
    }

    mutex.Lock()
    defer mutex.Unlock()

    fmt.Println("配置文件已生成")
}

func WriteBytes(filePath string, b []byte) (int, error) {
    os.MkdirAll(path.Dir(filePath), os.ModePerm)
    fw, err := os.Create(filePath)
    if err != nil{ return 0, err }
    defer fw.Close()
    return fw.Write(b)
}
