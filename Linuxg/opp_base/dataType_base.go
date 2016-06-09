package main

import (
	"fmt"
	"math"
	"math/cmplx"
	"strconv"
	"time"
	"unicode/utf8"
)

func integerBase() {
	fmt.Println("Integer arithmetic operation: ")
	var u uint8 = 255
	fmt.Println(u, u+1, u*u)
	var i int8 = 127
	fmt.Println(i, i+1, i*i)

	fmt.Println("Integer shift operation: ")
	var x uint8 = 1<<1 | 1<<5
	var y uint8 = 1<<1 | 1<<2
	fmt.Printf("%08b\n", x)
	fmt.Printf("%08b\n", y)
	fmt.Printf("%08b\n", x&y)
	fmt.Printf("%08b\n", x|y)
	fmt.Printf("%08b\n", x^y)
	fmt.Printf("%08b\n", x&^y)
	for i := uint(0); i < 8; i++ {
		if x&(1<<i) != 0 {
			fmt.Println(i)
		}
	}
	fmt.Printf("%08b\n", x<<1)
	fmt.Printf("%08b\n", x>>1)

	fmt.Println("Integer convert operation: ")
	var apples int32 = 1
	var oranges int16 = 2
	var compote = int(apples) + int(oranges)
	fmt.Println(compote)
	f := 3.141
	fmt.Println(f, int(f))

	fmt.Println("Integer radix format: ")
	o := 0666
	fmt.Printf("%d %[1]o %#[1]o\n", o)
	t := int64(0xdeadbeef)
	fmt.Printf("%d %[1]x %#[1]x %#[1]X\n", t)

	fmt.Println("Integer rune literals: ")
	ascii := 'a'
	unicode := '国'
	newline := '\n'
	fmt.Printf("%d %[1]c %[1]q\n", ascii)
	fmt.Printf("%d %[1]c %[1]q\n", unicode)
	fmt.Printf("%d %[1]q\n", newline)
	fmt.Println()
}

func floatBase() {
	fmt.Println("Float init value: ")
	const e = 2.71828
	fmt.Println(e)
	var z float64
	fmt.Println(z, -z, 1/z, -1/z, z/z)
	nan := math.NaN()
	fmt.Println(nan == nan, nan < nan, nan > nan)

	fmt.Println("Float math operation: ")
	for x := 0; x < 8; x++ {
		fmt.Printf("x = %d, e(x) = %8.3f\n", x, math.Exp(float64(x)))
	}
	fmt.Println()
}

func complexBase() {
	fmt.Println("Commplex arithmetic operation: ")
	var x complex128 = complex(1, 2)
	var y complex128 = complex(3, 4)
	fmt.Println(x * y)
	fmt.Println(real(x * y))
	fmt.Println(imag(x * y))

	fmt.Println("Commplex lib operation: ")
	fmt.Println(1i * 1i)
	fmt.Println(cmplx.Sqrt(-1))

	fmt.Println("Commplex compare operation: ")
	z := 1 + 2i
	t := 3 + 4i
	if z == t {
		fmt.Println("z == t")
	} else {
		fmt.Println("z != t")
	}
	fmt.Println()
}

func boolBase() {
	fmt.Println("Booleans logic operation: ")
	if (!true == false) == true {
		fmt.Println("Bool result is true")
	} else {
		fmt.Println("Bool result is false")
	}

	fmt.Println("Booleans init value: ")
	var x bool
	fmt.Println(x)

	fmt.Println("Booleans condition: ")
	s := "Hello, world"
	b := (s != "" && s[0] == 'x')
	if b {
		fmt.Println("string s is null or s[0]!=x")
	} else {
		fmt.Println("string s isn't null and s[0]=x")
	}

	fmt.Println("Booleans check valid value: ")
	t := '好'
	if 'a' <= t && t <= 'z' || 'A' <= t && t <= 'Z' || '0' <= t && t <= '9' {
		fmt.Println("t is valid char")
	} else {
		fmt.Println("t is valid char")
	}

	fmt.Println("Booleans to int: ")
	j := true
	if j {
		fmt.Println(1)
	} else {
		fmt.Println(0)
	}

	fmt.Println("Booleans return from int: ")
	g := 1
	if g == 1 {
		fmt.Println(true)
	} else {
		fmt.Println(false)
	}
	fmt.Println()
}

func stringBase() {
	fmt.Println("String base func: ")
	s := "hello, world"
	fmt.Println(len(s))
	fmt.Println(s[0:len(s)])
	fmt.Println(s[:])
	fmt.Println(s[0:7])
	fmt.Println(s[0], s[7])

	fmt.Println("String concat: ")
	fmt.Println("goodbye" + s[5:])
	g := "left foot"
	t := g
	g += ", right foot"
	fmt.Println(g)
	fmt.Println(t)
	fmt.Println()

	fmt.Println("String prefix or suffix find: ")
	x := "strncpy"
	y := "str"
	z := "cpy"
	if len(x) >= len(y) && x[0:len(y)] == y {
		fmt.Printf("string %q has prefix %q\n", x, y)
	} else if len(x) >= len(z) && x[len(x)-len(z):] == z {
		fmt.Printf("string %q has suffix %q\n", x, z)
	} else {
		fmt.Printf("string %q hasn't prefix %q or suffix %q\n", y, z)
	}

	fmt.Println("String lib use: ")
	f := "Hello, 世界"
	fmt.Println(len(f))
	fmt.Println(utf8.RuneCountInString(f))
	for i := 0; i < len(f); {
		r, size := utf8.DecodeRuneInString(f[i:])
		fmt.Printf("%d\t%c\t%q\t%d\n", i, r, r, r)
		i += size
	}
	n := 0
	for j, r := range f {
		fmt.Printf("%d\t%c\t%q\t%d\n", j, r, r, r)
		n++
	}

	fmt.Println("String to rune [UTF8 - Unicode]: ")
	c := "你好北京"
	fmt.Printf("% x\n", c)
	r := []rune(c)
	fmt.Printf("%x\n", r)
	fmt.Println(string(r))
	fmt.Println(string(0x4eac))
	fmt.Println(string(1234567))

	fmt.Println("String and number convert: ")
	a := 123
	b := fmt.Sprintf("%d", a)
	fmt.Println(b, strconv.Itoa(a))
	fmt.Println(strconv.FormatInt(int64(a), 2))
	fmt.Printf("%b\n", a)
	q, err := strconv.Atoi("123")
	if err == nil {
		fmt.Println(q)
	}
	w, err := strconv.ParseInt("123", 10, 64)
	if err == nil {
		fmt.Println(w)
	}

	fmt.Println()
}

func constantBase() {
	fmt.Println("Constant defer and use: ")
	const pi = 3.14159
	fmt.Println(pi)

	fmt.Println("Constant get type: ")
	const noDelay time.Duration = 0
	const timeout = 5 * time.Minute
	fmt.Printf("%T %[1]v\n", noDelay)
	fmt.Printf("%T %[1]v\n", timeout)
	fmt.Printf("%T %[1]v\n", time.Minute)

	fmt.Println("Constant implicite copy: ")
	const (
		a = 1
		b
		c = 2
		d
	)
	fmt.Println(a, b, c, d)

	fmt.Println("Constant itoa use: ")
	type Weekday int
	const (
		Sunday Weekday = iota
		Monday
		Tuesday
		Wednesday
		Thursday
		Friday
		Saturday
	)
	fmt.Println(Sunday, Monday, Tuesday, Wednesday, Thursday, Friday, Saturday)
	const (
		_ = 1 << (10 * iota)
		KB
		MB
		GB
		TB
		PB
	)
	fmt.Println(KB, MB, GB, TB, PB)
	fmt.Println()
}

func untypeBase() {
	fmt.Println("Untype var define: ")
	const Pi64 float64 = math.Pi
	var x float32 = float32(Pi64)
	var y float64 = Pi64
	var z complex128 = complex128(Pi64)
	fmt.Println(x, y, z)

	fmt.Println("Untype var arithmetic: ")
	var f float64 = 212
	fmt.Println((f - 32) * 5 / 9)
	fmt.Println(5 / 9 * (f - 32))
	fmt.Println(5.0 / 9.0 * (f - 32))

	fmt.Println("Untype var arithmetic: ")
	fmt.Printf("%T\n", 0)
	fmt.Printf("%T\n", 0.0)
	fmt.Printf("%T\n", 0i)
	fmt.Printf("%T\n", '\000')
	fmt.Println()
}
func main() {
	integerBase()
	floatBase()
	complexBase()
	boolBase()
	stringBase()
	constantBase()
	untypeBase()
}
