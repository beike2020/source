package main

import (
	"errors"
	"fmt"
	"os"
)

type PathError struct {
	Op   string
	Path string
	Err  error
}

func arg_func(args ...interface{}) {
	for _, arg := range args {
		switch arg.(type) {
		case int:
			fmt.Println(arg, "is an int value.")
		case string:
			fmt.Println(arg, "is a string value.")
		case int64:
			fmt.Println(arg, "is an int64 value.")
		default:
			fmt.Println(arg, "is an unknown type.")
		}
	}
	fmt.Println()
}

func close_func() {
	var j int = 5
	a := func() func() {
		var i int = 10
		return func() {
			fmt.Printf("i: %d, j: %d\n", i, j)
		}
	}()
	a()
	j *= 2
	a()
	fmt.Println()
}

func def_func(a, b int) (ret int, err error) {
	if a < 0 || b < 0 {
		err = errors.New("Should be non-negative numbers!")
		return
	}

	return a + b, nil
}

func (e *PathError) Error() string {
	return e.Op + " " + e.Path + ": " + e.Err.Error()
}

func Stat(name string) {
	fi, err := os.Stat(name)
	fd, err := os.Open(name)
	if err != nil {
		fmt.Println(&PathError{"stat", name, err})
		panic(err)
	} else {
		fmt.Println(fi.IsDir)
	}

	defer func() {
		if r := recover(); r != nil {
			fmt.Println("Runtime error caught: %v", r)
		}
		fd.Close()
	}()
}

func main() {
	var ret int
	var ok error
	var v1 int = 1
	var v2 int64 = 234
	var v3 string = "hello"
	var v4 float32 = 1.234

	arg_func(v1, v2, v3, v4)
	close_func()

	ret, ok = def_func(1, -2)
	if ok == nil {
		fmt.Println(ret)
		fmt.Println()
	} else {
		fmt.Println(ok)
		fmt.Println()
	}

	Stat("a.txt")
}
