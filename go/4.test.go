package main

import (
//        "bytes"
        "encoding/json"
//        "log"
//        "os"
        "fmt"
       )

func main() {
/*
    type Road struct {
        Name   string
        Number int
    }

    roads := []Road{
        {"Diamond Fork", 29},
        {"Sheep Creek", 51},
    }

    b, err := json.Marshal(roads)
    if err != nil {
        log.Fatalln(err)
    }

    var out bytes.Buffer
    err = json.Indent(&out, b, "", "    ")

    if err != nil {
        log.Fatalln(err)
    }

    out.WriteTo(os.Stdout)
*/
    c := make(map[string]interface{})
    c["name"] = "Gopher"
    c["title"] = "programmer"
    c["contact"] = map[string]interface{}{
        "home": "415.333.3333",
        "cell": "415.555.5555",
    }
    //var map1 map[string]map[string]string
    //map1 = make(map[string]map[string]string, 1)
    var map1 = make(map[string]map[string]string)
    map1["ptjy/ptyw"] = make(map[string]string, 1)
    map1["ptjy/ptyw"]["login"] = "ptjy_ptyw_login"
    map1["ptjy/ptyw"]["cccx"] = "ptjy_ptyw_cccx"
    var str = [5]string{0: "qqq", 1:"www", 2:"eee", 3:"rrr", 4:"ttt"}
    for i := 0; i < 5; i++ {
        map1["ptjy/ptyw"][str[i]] = "ptjy_ptyw_cccx"
    
    }

    // 将这个映射序列化到JSON 字符串
    data, err := json.MarshalIndent(map1, "", "    ")
    if err != nil {
        fmt.Println("ERROR:", err)
    }
    
    fmt.Println(string(data))
}

