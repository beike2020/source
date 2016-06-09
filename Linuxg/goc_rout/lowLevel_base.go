package main

import (
	"bytes"
	"compress/bzip2" // reader
	"fmt"
	"io"
	"log"
	"os"
	"reflect"
	"unsafe"
)

/*
#cgo CFLAGS: -I/usr/include
#cgo LDFLAGS: -L/usr/lib -lbz2
#include <bzlib.h>
#include <stdlib.h>
bz_stream* bz2alloc() { return calloc(1, sizeof(bz_stream)); }
int bz2compress(bz_stream *s, int action, char *in, unsigned *inlen, char *out, unsigned *outlen);
void bz2free(bz_stream* s) { free(s); }
*/
import "C"

type writer struct {
	w      io.Writer // underlying output stream
	stream *C.bz_stream
	outbuf [64 * 1024]byte
}

type comparison struct {
	x, y unsafe.Pointer
	t    reflect.Type
}

type link struct {
	value string
	tail  *link
}

func NewWriterOne(out io.Writer) io.WriteCloser {
	const (
		blockSize  = 9
		verbosity  = 0
		workFactor = 30
	)
	w := &writer{w: out, stream: C.bz2alloc()}
	C.BZ2_bzCompressInit(w.stream, blockSize, verbosity, workFactor)
	return w
}

func NewWriterThree(out io.Writer) io.WriteCloser {
	const (
		blockSize  = 9
		verbosity  = 0
		workFactor = 30
	)
	w := &writer{w: out, stream: new(C.bz_stream)}
	C.BZ2_bzCompressInit(w.stream, blockSize, verbosity, workFactor)
	return w
}

func (w *writer) Write(data []byte) (int, error) {
	var total int

	if w.stream == nil {
		panic("closed")
	}

	for len(data) > 0 {
		inlen, outlen := C.uint(len(data)), C.uint(cap(w.outbuf))
		C.bz2compress(w.stream, C.BZ_RUN,
			(*C.char)(unsafe.Pointer(&data[0])), &inlen,
			(*C.char)(unsafe.Pointer(&w.outbuf)), &outlen)
		total += int(inlen)
		data = data[inlen:]
		if _, err := w.w.Write(w.outbuf[:outlen]); err != nil {
			return total, err
		}
	}
	return total, nil
}

func (w *writer) Close() error {
	if w.stream == nil {
		panic("closed")
	}
	defer func() {
		C.BZ2_bzCompressEnd(w.stream)
		C.bz2free(w.stream)
		w.stream = nil
	}()
	for {
		inlen, outlen := C.uint(0), C.uint(cap(w.outbuf))
		r := C.bz2compress(w.stream, C.BZ_FINISH, nil, &inlen,
			(*C.char)(unsafe.Pointer(&w.outbuf)), &outlen)
		if _, err := w.w.Write(w.outbuf[:outlen]); err != nil {
			return err
		}
		if r == C.BZ_STREAM_END {
			return nil
		}
	}
}

func TestBzipOne() {
	var compressed, uncompressed, decompressed bytes.Buffer

	w := bzip.NewWriterOne(&compressed)
	tee := io.MultiWriter(w, &uncompressed)
	for i := 0; i < 1000000; i++ {
		io.WriteString(tee, "hello")
	}
	if err := w.Close(); err != nil {
		fmt.Println(err)
		return
	}

	if got, want := compressed.Len(), 255; got != want {
		fmt.Printf("1 million hellos compressed to %d bytes, want %d\n", got, want)
	}

	io.Copy(&decompressed, bzip2.NewReader(&compressed))
	if !bytes.Equal(uncompressed.Bytes(), decompressed.Bytes()) {
		fmt.Println("decompression yielded a different message")
	}
}

func TestBzipTwo() {
	w := bzip.NewWriterOne(os.Stdout)
	if _, err := io.Copy(w, os.Stdin); err != nil {
		log.Fatalf("bzipper: %v\n", err)
	}
	if err := w.Close(); err != nil {
		log.Fatalf("bzipper: close: %v\n", err)
	}
}

func TestBzipThree() {
	var compressed, uncompressed bytes.Buffer

	w := bzip.NewWriterThree(&compressed)
	tee := io.MultiWriter(w, &uncompressed)
	for i := 0; i < 1000000; i++ {
		io.WriteString(tee, "hello")
	}
	if err := w.Close(); err != nil {
		fmt.Println(err)
	}

	if got, want := compressed.Len(), 255; got != want {
		fmt.Printf("1 million hellos compressed to %d bytes, want %d\n", got, want)
	}

	var decompressed bytes.Buffer
	io.Copy(&decompressed, bzip2.NewReader(&compressed))
	if !bytes.Equal(uncompressed.Bytes(), decompressed.Bytes()) {
		fmt.Println("decompression yielded a different message")
	}
}

func TestUnsafe() {
	var x struct {
		a bool
		b int16
		c []int
	}

	pb := (*int16)(unsafe.Pointer(uintptr(unsafe.Pointer(&x)) + unsafe.Offsetof(x.b)))
	*pb = 42
	fmt.Println(x.b)
}

func Equal(x, y interface{}) bool {
	seen := make(map[comparison]bool)
	return equal(reflect.ValueOf(x), reflect.ValueOf(y), seen)
}

func equal(x, y reflect.Value, seen map[comparison]bool) bool {
	if !x.IsValid() || !y.IsValid() {
		return x.IsValid() == y.IsValid()
	}

	if x.Type() != y.Type() {
		return false
	}

	if x.CanAddr() && y.CanAddr() {
		xptr := unsafe.Pointer(x.UnsafeAddr())
		yptr := unsafe.Pointer(y.UnsafeAddr())
		if xptr == yptr {
			return true
		}
		c := comparison{xptr, yptr, x.Type()}
		if seen[c] {
			return true
		}
		seen[c] = true
	}

	switch x.Kind() {
	case reflect.Bool:
		return x.Bool() == y.Bool()

	case reflect.String:
		return x.String() == y.String()

	case reflect.Int, reflect.Int8, reflect.Int16, reflect.Int32,
		reflect.Int64:
		return x.Int() == y.Int()

	case reflect.Uint, reflect.Uint8, reflect.Uint16, reflect.Uint32,
		reflect.Uint64, reflect.Uintptr:
		return x.Uint() == y.Uint()

	case reflect.Float32, reflect.Float64:
		return x.Float() == y.Float()

	case reflect.Complex64, reflect.Complex128:
		return x.Complex() == y.Complex()

	case reflect.Chan, reflect.UnsafePointer, reflect.Func:
		return x.Pointer() == y.Pointer()

	case reflect.Ptr, reflect.Interface:
		return equal(x.Elem(), y.Elem(), seen)

	case reflect.Array, reflect.Slice:
		if x.Len() != y.Len() {
			return false
		}
		for i := 0; i < x.Len(); i++ {
			if !equal(x.Index(i), y.Index(i), seen) {
				return false
			}
		}
		return true

	case reflect.Struct:
		for i, n := 0, x.NumField(); i < n; i++ {
			if !equal(x.Field(i), y.Field(i), seen) {
				return false
			}
		}
		return true

	case reflect.Map:
		if x.Len() != y.Len() {
			return false
		}
		for _, k := range x.MapKeys() {
			if !equal(x.MapIndex(k), y.MapIndex(k), seen) {
				return false
			}
		}
		return true
	}
	panic("unreachable")
}

func TestEqual() {
	one, oneAgain, two := 1, 1, 2

	type CyclePtr *CyclePtr
	var cyclePtr1, cyclePtr2 CyclePtr
	cyclePtr1 = &cyclePtr1
	cyclePtr2 = &cyclePtr2

	type CycleSlice []CycleSlice
	var cycleSlice CycleSlice
	cycleSlice = append(cycleSlice, cycleSlice)

	ch1, ch2 := make(chan int), make(chan int)
	var ch1ro <-chan int = ch1

	type mystring string

	var iface1, iface1Again, iface2 interface{} = &one, &oneAgain, &two

	for _, test := range []struct {
		x, y interface{}
		want bool
	}{
		// basic types
		{1, 1, true},
		{1, 2, false},   // different values
		{1, 1.0, false}, // different types
		{"foo", "foo", true},
		{"foo", "bar", false},
		{mystring("foo"), "foo", false}, // different types
		// slices
		{[]string{"foo"}, []string{"foo"}, true},
		{[]string{"foo"}, []string{"bar"}, false},
		{[]string{}, []string(nil), true},
		// slice cycles
		{cycleSlice, cycleSlice, true},
		// maps
		{
			map[string][]int{"foo": {1, 2, 3}},
			map[string][]int{"foo": {1, 2, 3}},
			true,
		},
		{
			map[string][]int{"foo": {1, 2, 3}},
			map[string][]int{"foo": {1, 2, 3, 4}},
			false,
		},
		{
			map[string][]int{},
			map[string][]int(nil),
			true,
		},
		// pointers
		{&one, &one, true},
		{&one, &two, false},
		{&one, &oneAgain, true},
		{new(bytes.Buffer), new(bytes.Buffer), true},
		// pointer cycles
		{cyclePtr1, cyclePtr1, true},
		{cyclePtr2, cyclePtr2, true},
		{cyclePtr1, cyclePtr2, true}, // they're deeply equal
		// functions
		{(func())(nil), (func())(nil), true},
		{(func())(nil), func() {}, false},
		{func() {}, func() {}, false},
		// arrays
		{[...]int{1, 2, 3}, [...]int{1, 2, 3}, true},
		{[...]int{1, 2, 3}, [...]int{1, 2, 4}, false},
		// channels
		{ch1, ch1, true},
		{ch1, ch2, false},
		{ch1ro, ch1, false}, // NOTE: not equal
		// interfaces
		{&iface1, &iface1, true},
		{&iface1, &iface2, false},
		{&iface1Again, &iface1, true},
	} {
		if Equal(test.x, test.y) != test.want {
			fmt.Printf("Error: Equal(%v, %v) = %t\n", test.x, test.y, !test.want)
		}
	}

	fmt.Println(Equal([]int{1, 2, 3}, []int{1, 2, 3}))
	fmt.Println(Equal([]string{"foo"}, []string{"bar"}))
	fmt.Println(Equal([]string(nil), []string{}))
	fmt.Println(Equal(map[string]int(nil), map[string]int{}))

	a, b, c := &link{value: "a"}, &link{value: "b"}, &link{value: "c"}
	a.tail, b.tail, c.tail = b, a, c
	fmt.Println(Equal(a, a))
	fmt.Println(Equal(b, b))
	fmt.Println(Equal(c, c))
	fmt.Println(Equal(a, b))
	fmt.Println(Equal(a, c))
}

func main() {
	TestBzipOne()
	TestBzipTwo()
	TestBzipThree()
	TestUnsafe()
	TestEqual()
}
