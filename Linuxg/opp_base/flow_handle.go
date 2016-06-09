package main

import "fmt"

func if_flow(x int) int {
	ret := 0
	if x == 0 {
		ret = 0
	} else {
		ret = 1
	}
	fmt.Println(ret)
	fmt.Println()

	return ret
}

func case_flow(x int) {
	switch x {
	case 0:
		fmt.Println("0")
	case 1:
		fallthrough
	case 2:
		fmt.Println("1")
	case 3, 4, 5:
		fmt.Println("3, 4, 5")
	default:
		fmt.Println("Default")
	}

	switch {
	case 0 <= x && x <= 2:
		fmt.Println("0-2")
	case 3 <= x && x <= 5:
		fmt.Println("3-5")
	case 0 > x || 6 <= x:
		fmt.Println("Default")
	}
	fmt.Println()
}

func for_flow() {
	a := []int{1, 2, 3, 4, 5, 6}

	fmt.Print("Elements of array: ")
	for v := range a {
		fmt.Print(v, " ")
	}
	fmt.Println()

	for i, j := 0, len(a)-1; i < j; i, j = i+1, j-1 {
		fmt.Println("i=", i, "j=", j)
		fmt.Println("a[i]=", a[i], "a[j]=", a[j])
		a[i], a[j] = a[j], a[i]
		fmt.Println("exchanged: a[i]=", a[i], "a[j]=", a[j])
	}

	fmt.Print("Elements of array: ")
JLoop:
	for _, v := range a {
		if v < 3 {
			break JLoop
		}
		fmt.Print(v, " ")
	}
	fmt.Println()
	fmt.Println()
}

func goto_flow() {
	i := 0
	fmt.Print("Elements of array: ")
HERE:
	fmt.Print(i, " ")
	i++
	if i < 10 {
		goto HERE
	}
	fmt.Println()
	fmt.Println()
}

func main() {
	if_flow(1)
	case_flow(3)
	for_flow()
	goto_flow()

}
