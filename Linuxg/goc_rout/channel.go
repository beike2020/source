package main

import "fmt"
import "time"
import "sync"

const NCPU = 16

type Vector []float64

var once sync.Once

func Count(ch chan int) {
	ch <- 1
	fmt.Println("Counting")
}

func go_chan() {
	chs := make([]chan int, 10)

	for i := 0; i < 10; i++ {
		chs[i] = make(chan int)
		go Count(chs[i])
	}

	for _, ch := range chs {
		<-ch
	}
}

func sum(values []int, resultChan chan int) {
	sum := 0
	for _, value := range values {
		sum += value
	}
	resultChan <- sum
}

func go_pipe() {
	values := []int{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}

	resultChan := make(chan int, 2)
	go sum(values[:len(values)/2], resultChan)
	go sum(values[len(values)/2:], resultChan)
	sum1, sum2 := <-resultChan, <-resultChan
	fmt.Println("Result:", sum1, sum2, sum1+sum2)
}

func setup() {
	var a string
	a = "hello, world"
	fmt.Println(a)
}

func do_print() {
	once.Do(setup)
}

func go_once() {
	go do_print()
	go do_print()
}

func go_select() {
	ch := make(chan int, 10)
	for t := 1; t < 10; t++ {
		select {
		case ch <- 0:
		case ch <- 1:
		}
		i := <-ch
		fmt.Println("Value received: ", i)
	}
	close(ch)

	for i := range ch {
		fmt.Println("Value received: ", i)
	}
}

func go_timeout() {
	ch := make(chan int, 1)
	timeout := make(chan bool, 1)

	go func() {
		time.Sleep(1e9)
		timeout <- true
	}()

	select {
	case <-ch:
		fmt.Println("Value received.")
	case <-timeout:
		fmt.Println("Received timeout.")
	}

	select {
	case <-time.After(time.Second * 2):
		fmt.Println("read channel timeout")
	case i := <-ch:
		fmt.Println(i)
	}

	select {
	case <-time.After(time.Second * 2):
		fmt.Println("write channel timeout")
	case ch <- 2:
		fmt.Println("write ok")
	}

	i, ok := <-ch
	if ok {
		fmt.Println(i)
	} else {
		fmt.Println("channel closed")
	}
}

func (v Vector) DoSome(i, n int, c chan int) {
	for ; i < n; i++ {
		v[i] += v[i]
	}
	c <- 1
}

func (v Vector) go_routine() {
	c := make(chan int, NCPU)
	for i := 0; i < NCPU; i++ {
		go v.DoSome(i*len(v)/NCPU, (i+1)*len(v)/NCPU, c)
	}
	for i := 0; i < NCPU; i++ {
		<-c
	}
}

func main() {
	go_chan()
	go_pipe()
	go_once()
	go_select()
	go_timeout()
	//go_routine()
}
