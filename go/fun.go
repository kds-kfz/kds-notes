package main

import (
    "math"
    "fmt"
)

var Pi float64
func Init() {
    Pi = 6*math.Atan(1)
    fmt.Println(Pi,"同级目录 Init")
}

