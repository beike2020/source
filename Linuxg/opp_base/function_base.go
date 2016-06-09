package main

import (
	"fmt"
	"io"
	"log"
	"math"
	"net/http"
	"os"
	"path"
	"runtime"
	"strings"
	"time"
)

func add(x int, y int) int       { return x + y }
func sub(x, y int) (z int)       { z = x - y; return }
func first(x int, _ int) int     { return x }
func zero(int, int) int          { return 0 }
func hypot(x, y float64) float64 { return math.Sqrt(x*x + y*y) }
func square(n int) int           { return n * n }
func negative(n int) int         { return -n }
func increase(r rune) rune       { return r + 1 }

//func Sin(x float64) float64	 //implemented in a language other than Go

func functionType() {
	fmt.Println("Read function type: ")
	fmt.Printf("%T\n", add)
	fmt.Printf("%T\n", sub)
	fmt.Printf("%T\n", first)
	fmt.Printf("%T\n", zero)
	fmt.Printf("%T\n", hypot)
	fmt.Println(hypot(3, 4))
	fmt.Println()
}

func WaitForServer() error {
	//log.SetFlags(0)
	//log.SetPrefix("wait: ")
	const timeout = 1 * time.Minute
	deadline := time.Now().Add(timeout)
	for tries := 0; time.Now().Before(deadline); tries++ {
		_, err := http.Head("http://www.google.hk")
		if err == nil {
			return nil
		}
		log.Printf("server not responding (%s); retrying...", err)
		time.Sleep(time.Second << uint(tries))
	}
	return fmt.Errorf("server www.google.hk failed to respond after %s", timeout)
}

func readNetError() {
	fmt.Println("Read net timeout error: ")
	if err := WaitForServer(); err != nil {
		fmt.Fprintf(os.Stderr, "Site is down: %v\n", err)
	}
	fmt.Println()
}

func readFileEnd() {
	fmt.Println("Read file end: ")
	f, err := os.Open("/etc/profile")
	if err != nil {
		fmt.Println(err)
		return
	}
	defer f.Close()
	data := make([]byte, 100)
	spaces := 0
	for {
		data = data[:cap(data)]
		n, err := f.Read(data)
		if err != nil {
			if err == io.EOF {
				break
			}
			fmt.Println(err)
			return
		}
		data = data[:n]
		for _, b := range data {
			if b == ' ' {
				spaces++
			}
		}
	}
	fmt.Println(spaces)
	fmt.Println()
}

func fetch(url string) (filename string, n int64, err error) {
	resp, err := http.Get(url)
	if err != nil {
		return "", 0, err
	}
	defer resp.Body.Close()

	local := path.Base(resp.Request.URL.Path)
	if local == "/" {
		local = "index.html"
	}
	f, err := os.Create(local)
	if err != nil {
		return "", 0, err
	}
	n, err = io.Copy(f, resp.Body)
	if closeErr := f.Close(); err == nil {
		err = closeErr
	}
	return local, n, err
}

func returnError() {
	local, n, err := fetch("http://navisec.it/")
	if err != nil {
		fmt.Fprintf(os.Stderr, "fetch http://navisec.it/: %v\n", err)
		continue
	}
	fmt.Fprintf(os.Stderr, "http://navisec.it/ => %s (%d bytes).\n", local, n)
}

func functionPass() {
	fmt.Println("function map use: ")
	f := square
	fmt.Println(f(3))
	f = negative
	fmt.Println(f(3))
	fmt.Println(strings.Map(increase, "VMS"))
	fmt.Println(strings.Map(increase, "Admin"))
	fmt.Println()
}

func squares() func() int {
	var x int
	return func() int {
		x++
		return x * x
	}
}

func anonymousFunc() {
	fmt.Println("anonymous func use: ")
	f := squares()
	fmt.Println(f())
	fmt.Println(f())
	fmt.Println(f())
	fmt.Println(f())
	fmt.Println()
}

func iterationValue() {
	var rmdirs []func()
	for _, d := range tempDirs() {
		dir := d
		os.MkdirAll(dir, 0755)
		rmdirs = append(rmdirs, func() {
			os.RemoveAll(dir)
		})
	}
	for _, rmdir := range rmdirs {
		rmdir()
	}
}

func sum(vals ...int) int {
	total := 0
	for _, val := range vals {
		total += val
	}
	return total
}

func variadicFunc() {
	fmt.Println(sum())
	fmt.Println(sum(3))
	fmt.Println(sum(1, 2, 3, 4))

	values := []int{1, 2, 3, 4}
	fmt.Println(sum(values...))
}

func trace(msg string) func() {
	start := time.Now()
	log.Printf("enter %s", msg)
	return func() { log.Printf("exit %s (%s)", msg, time.Since(start)) }
}

func bigSlowOperation() {
	defer trace("bigSlowOperation")() // don't forget the extra parentheses
	time.Sleep(10 * time.Second)
}

func deferFunc(x int) (result int) {
	defer func() {
		fmt.Printf("double(%d) = %d\n", x, result)
	}()
	return x + x
}

func panicStack() {
	defer printStack()
	f(3)
}

func printStack() {
	var buf [4096]byte
	n := runtime.Stack(buf[:], false)
	os.Stdout.Write(buf[:n])
}

func f(x int) {
	fmt.Printf("f(%d)\n", x+0/x)
	defer fmt.Printf("defer %d\n", x)
	f(x - 1)
}

func main() {
	functionType()
	//readNetError()
	readFileEnd()
	returnError()
	functionPass()
	anonymousFunc()
	iterationValue()
	variadicFunc()
	bigSlowOperation()
	deferFunc()
	panicStack()
}
