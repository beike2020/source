//usage: GOMAXPROCS=5 go run concurrentShare_base.go
package main

import (
	"fmt"
	"sync"
)

var (
	balance  int
	deposits = make(chan int) // send amount to deposit
	balances = make(chan int) // receive balance
	sema     = make(chan struct{}, 1)
	mu       sync.Mutex
	murw     sync.RWMutex
	loadOnce sync.Once
	icons    map[string]string
)

func SetMaxProc() {
	for i := 0; i < 10; i++ {
		go fmt.Println(0)
		fmt.Println(1)
	}
}

func DepositOne(amount int) {
	deposits <- amount
}

func BalanceOne() int {
	return <-balances
}

func teller() {
	for {
		select {
		case amount := <-deposits:
			balance += amount
		case balances <- balance:
		}
	}
}

func TestBankOne() {
	balance = 0
	done := make(chan struct{})
	go teller()

	// Alice
	go func() {
		DepositOne(200)
		done <- struct{}{}
	}()

	// Bob
	go func() {
		DepositOne(100)
		done <- struct{}{}
	}()

	// Wait for both transactions.
	<-done
	<-done

	if got, want := BalanceOne(), 300; got != want {
		fmt.Printf("Error-One: Balance = %d, want = %d\n", got, want)
	}
}

func DepositTwo(amount int) {
	sema <- struct{}{} // acquire token
	balance = balance + amount
	<-sema // release token
}

func BalanceTwo() int {
	sema <- struct{}{} // acquire token
	b := balance
	<-sema // release token
	return b
}

func TestBankTwo() {
	balance = 0
	var n sync.WaitGroup
	for i := 1; i <= 1000; i++ {
		n.Add(1)
		go func(amount int) {
			DepositTwo(amount)
			n.Done()
		}(i)
	}
	n.Wait()

	if got, want := BalanceTwo(), (1000+1)*1000/2; got != want {
		fmt.Printf("Error-Two: Balance = %d, want %d\n", got, want)
	}
}

func DepositThree(amount int) {
	mu.Lock()
	balance = balance + amount
	mu.Unlock()
}

func BalanceThree() int {
	mu.Lock()
	b := balance
	mu.Unlock()
	return b
}

func TestBankThree() {
	balance = 0
	var n sync.WaitGroup
	for i := 1; i <= 1000; i++ {
		n.Add(1)
		go func(amount int) {
			DepositThree(amount)
			n.Done()
		}(i)
	}
	n.Wait()

	if got, want := BalanceThree(), (1000+1)*1000/2; got != want {
		fmt.Printf("Error-Three: Balance = %d, want %d\n", got, want)
	}
}

func DepositFour(amount int) {
	murw.Lock()
	balance = balance + amount
	murw.Unlock()
}

func BalanceFour() int {
	murw.RLock()
	b := balance
	murw.RUnlock()
	return b
}

func TestBankFour() {
	balance = 0
	var n sync.WaitGroup
	for i := 1; i <= 1000; i++ {
		n.Add(1)
		go func(amount int) {
			DepositFour(amount)
			n.Done()
		}(i)
	}
	n.Wait()

	if got, want := BalanceFour(), (1000+1)*1000/2; got != want {
		fmt.Printf("Error-Four: Balance = %d, want %d\n", got, want)
	}
}

func loadIcon(name string) string {
	return name
}

func loadIcons() {
	icons = map[string]string{
		"spades.png":   loadIcon("spades.png"),
		"hearts.png":   loadIcon("hearts.png"),
		"diamonds.png": loadIcon("diamonds.png"),
		"clubs.png":    loadIcon("clubs.png"),
	}
}

func TestIcon(name string) string {
	loadOnce.Do(loadIcons)
	return icons[name]
}

func main() {
	SetMaxProc()
	TestBankOne()
	TestBankTwo()
	TestBankThree()
	TestBankFour()
	TestIcon("clubs.png")
}
