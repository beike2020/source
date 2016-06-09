package main

import (
	"fmt"
	"os"
	"strings"
)

func args_show() {
	var s, sep string
	for i := 1; i < len(os.Args); i++ {
		s += sep + os.Args[i]
		sep = " "
	}
	fmt.Println(s)
}

func args_cats() {
	s, sep := "", ""
	for _, arg := range os.Args[1:] {
		s += sep + arg
		sep = " "
	}
	fmt.Println(s)
}

func args_join() {
	fmt.Println(strings.Join(os.Args[1:], " "))
}

func main() {
	args_show()
	args_cats()
	args_join()
}
