package memo

import (
	"fmt"
	"io/ioutil"
	"log"
	"net/http"
	"sync"
	"testing"
	"time"
)

type Func func(string) (interface{}, error)

type result struct {
	value interface{}
	err   error
}

type entry struct {
	res   result
	ready chan struct{}
}

type request struct {
	key      string
	response chan<- result
}

type MemoR struct {
	f     Func
	mu    sync.Mutex
	cache map[string]result
}

type MemoE struct {
	f     Func
	mu    sync.Mutex
	cache map[string]*entry
}

type MemoT struct {
	requests chan request
}

func NewR(f Func) *MemoR {
	return &MemoR{f: f, cache: make(map[string]result)}
}

func NewE(f Func) *MemoE {
	return &MemoE{f: f, cache: make(map[string]*entry)}
}

func NewT(f Func) *MemoT {
	memo := &Memo{requests: make(chan request)}
	go memo.server(f)
	return memo
}

func (memo *MemoR) GetOne(key string) (value interface{}, err error) {
	memo.mu.Lock()
	res, ok := memo.cache[key]
	if !ok {
		res.value, res.err = memo.f(key)
		memo.cache[key] = res
	}
	memo.mu.Unlock()
	return res.value, res.err
}

func (memo *MemoR) GetTwo(key string) (value interface{}, err error) {
	memo.mu.Lock()
	res, ok := memo.cache[key]
	memo.mu.Unlock()
	if !ok {
		res.value, res.err = memo.f(key)
		memo.mu.Lock()
		memo.cache[key] = res
		memo.mu.Unlock()
	}
	return res.value, res.err
}

func (memo *MemoE) GetThree(key string) (value interface{}, err error) {
	memo.mu.Lock()
	e := memo.cache[key]
	if e == nil {
		e = &entry{ready: make(chan struct{})}
		memo.cache[key] = e
		memo.mu.Unlock()
		e.res.value, e.res.err = memo.f(key)
		close(e.ready)
	} else {
		memo.mu.Unlock()
		<-e.ready
	}
	return e.res.value, e.res.err
}

func (memo *MemoT) GetThree(key string) (interface{}, error) {
	response := make(chan result)
	memo.requests <- request{key, response}
	res := <-response
	return res.value, res.err
}

func (memo *MemoT) Close() {
	close(memo.requests)
}

func (e *entry) call(f Func, key string) {
	e.res.value, e.res.err = f(key)
	close(e.ready)
}

func (e *entry) deliver(response chan<- result) {
	<-e.ready
	response <- e.res
}

func (memo *MemoT) server(f Func) {
	cache := make(map[string]*entry)
	for req := range memo.requests {
		e := cache[req.key]
		if e == nil {
			e = &entry{ready: make(chan struct{})}
			cache[req.key] = e
			go e.call(f, req.key)
		}
		go e.deliver(req.response)
	}
}

func httpGetBody(url string) (interface{}, error) {
	resp, err := http.Get(url)
	if err != nil {
		return nil, err
	}
	defer resp.Body.Close()
	return ioutil.ReadAll(resp.Body)
}

func incomingURLs() <-chan string {
	ch := make(chan string)
	go func() {
		for _, url := range []string{
			"https://golang.org",
			"https://godoc.org",
			"https://play.golang.org",
			"http://gopl.io",
			"https://golang.org",
			"https://godoc.org",
			"https://play.golang.org",
			"http://gopl.io",
		} {
			ch <- url
		}
		close(ch)
	}()
	return ch
}

type M interface {
	Get(key string) (interface{}, error)
}

func Sequential(m M) {
	for url := range incomingURLs() {
		start := time.Now()
		value, err := m.Get(url)
		if err != nil {
			log.Print(err)
			continue
		}
		fmt.Printf("%s, %s, %d bytes\n", url, time.Since(start), len(value.([]byte)))
	}
}

func Concurrent(m M) {
	var n sync.WaitGroup
	for url := range incomingURLs() {
		n.Add(1)
		go func(url string) {
			defer n.Done()
			start := time.Now()
			value, err := m.Get(url)
			if err != nil {
				log.Print(err)
				return
			}
			fmt.Printf("%s, %s, %d bytes\n", url, time.Since(start), len(value.([]byte)))
		}(url)
	}
	n.Wait()
}

func TestOne() {
	m := NewR(httpGetBody)
	Sequential(m)
}

func TestConcurrentOne() {
	m := NewR(httpGetBody)
	Concurrent(m)
}

func TestTwo() {
	m := NewE(httpGetBody)
	Sequential(m)
}

func TestConcurrentTwo() {
	m := NewE(httpGetBody)
	Concurrent(m)
}
func TestThree() {
	m := NewT(httpGetBody)
	defer m.Close()
	Sequential(m)
}

func TestConcurrentThree() {
	m := NewT(httpGetBody)
	defer m.Close()
	Concurrent(m)
}

func main() {
	TestOne()
	TestConcurrentOne()
	TestTwo()
	TestConcurrentTwo()
	TestThree()
	TestConcurrentThree()
}
