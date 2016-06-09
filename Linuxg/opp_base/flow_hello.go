package main

/*
#include <stdio.h>
#include <stdlib.h>
*/
import "C"
import "fmt"
import "unsafe"

func hello() {
	fmt.Println("Hello, world. 你好，世界！")
}

func Foo(a, b int) (ret int, err error) {
	if a > b {
		return a, nil
	} else {
		return b, nil
	}
	return 0, nil
}

func ctogo() {
	cstr := C.CString("Hello, world")
	C.puts(cstr)
	C.free(unsafe.Pointer(cstr))
}

func main() {
	hello()
	i, _ := Foo(1, 2)
	fmt.Println("Hello, 世界", i)
	ctogo()
}
