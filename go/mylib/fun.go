package mylib

import (
    "math"
    "fmt"
)

var Pi float64
func Init() {
    Pi = 4*math.Atan(1)
    fmt.Println(Pi,"mylib 包中的 Init")
}

