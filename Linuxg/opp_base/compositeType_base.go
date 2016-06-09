package main

import (
	"encoding/json"
	"fmt"
	"html/template"
	"log"
	"math/rand"
	"os"
	"sort"
)

type Point struct {
	X, Y int
}
type Circle struct {
	Point
	Radius int
}
type Wheel struct {
	Circle
	Spokes int
}

type Movie struct {
	Title  string
	Year   int  `json:"released"`
	Color  bool `json:"color,omitempty"`
	Actors []string
}

var movies = []Movie{
	{Title: "Casablanca", Year: 1942, Color: false,
		Actors: []string{"Humphrey Bogart", "Ingrid Bergman"}},
	{Title: "Cool Hand Luke", Year: 1967, Color: true,
		Actors: []string{"Paul Newman"}},
	{Title: "Bullitt", Year: 1968, Color: true,
		Actors: []string{"Steve McQueen", "Jacqueline Bisset"}},
}

type tree struct {
	value       int
	left, right *tree
}

func arrayBase() {
	fmt.Println("Array type use: ")
	var a [3]int
	fmt.Println(a[0], a[len(a)-1])
	for i, v := range a {
		fmt.Printf("%d %d\n", i, v)
	}
	for _, t := range a {
		fmt.Printf("%d\n", t)
	}

	fmt.Println("Array init: ")
	var b [3]int = [3]int{1, 2}
	fmt.Println(b[2])
	c := [...]int{1, 2, 3}
	z := [...]int{10: -1}
	fmt.Printf("%T\n", c)
	fmt.Println(z)

	fmt.Println("Array compare: ")
	d := [2]int{1, 2}
	e := [...]int{1, 2}
	f := [2]int{3, 4}
	fmt.Println(d == e, e == f, d == f)

	type Currency int
	const (
		USD Currency = iota
		EUR
		GBP
		RMB
	)
	symbol := [...]string{USD: "$", EUR: "@", GBP: "#", RMB: "&"}
	fmt.Println(RMB, symbol[RMB])
	fmt.Println()
}

func sliceBase() {
	fmt.Println("Slice define: ")
	months := [...]string{0: "", 1: "January", 2: "February", 3: "March", 4: "April", 5: "May", 6: "June", 7: "July", 8: "August", 9: "September", 10: "October", 11: "November", 12: "December"}
	Q2 := months[4:7]
	summer := months[6:9]
	fmt.Println(Q2)
	fmt.Println(summer)
	endless := summer[:5]
	fmt.Println(endless)

	fmt.Println("Slice init: ")
	var a []int
	a = nil
	fmt.Printf("len(a) is %d, a is nil: %t\n", len(a), a == nil)
	a = []int(nil)
	fmt.Printf("len(a) is %d, a is nil: %t\n", len(a), a == nil)
	a = []int{}
	fmt.Printf("len(a) is %d, a is nil: %t\n", len(a), a == nil)
	var b = make([]int, 1, 10)
	fmt.Printf("len(b) is %d, cap(b) is %d\n", len(b), cap(b))

	fmt.Println("Slice append: ")
	var runes []rune
	for _, r := range "Hello, 世界" {
		runes = append(runes, r)
	}
	fmt.Printf("%q\n", runes)
	var x []int
	x = append(x, 1)
	x = append(x, 2, 3)
	x = append(x, 4, 5, 6)
	x = append(x, x...)
	fmt.Println(x)

	fmt.Println("Slice compare: ")
	for _, s := range summer {
		for _, q := range Q2 {
			if s == q {
				fmt.Printf("%s appears in both\n", s)
			}
		}
	}

	fmt.Println()
}

func mapsBase() {
	fmt.Println("Slice init: ")
	ages := make(map[string]int)
	ages["alice"] = 31
	ages["charlie"] = 34
	ages["beike"] = 26
	ages["tom"] = 32
	ages["bob"] = 39
	ages["charlie"]++
	fmt.Println(ages["charlie"], ages["hello"])

	fmt.Println("Slice item get: ")
	age, ok := ages["bob"]
	if !ok {
		fmt.Printf("ages[\"bob\"] is %d\n", age)
	} else {
		fmt.Println("ages[\"bob\"] isn't exist")
	}

	fmt.Println("Slice iteration no order: ")
	for name, age := range ages {
		fmt.Printf("%s\t%d\n", name, age)
	}

	fmt.Println("Slice iteration in sort: ")
	var names []string
	for n := range ages {
		names = append(names, n)
	}
	sort.Strings(names)
	for _, name := range names {
		fmt.Printf("%s\t%d\n", name, ages[name])
	}

	fmt.Println("Slice iteration compare: ")
	tems := map[string]int{
		"alice":   31,
		"charlie": 34,
		"beike":   26,
		"tom":     29,
		"gale":    28,
	}
	f := 0
	if len(ages) == len(tems) {
		for k, kv := range ages {
			if yv, ok := tems[k]; !ok || yv != kv {
				fmt.Println("ages isn't equal to tems")
				break
			}
			f++
		}
		if f > len(ages) {
			fmt.Println("ages is equal to tems")
		}
	} else {
		fmt.Println("ages isn't equal to tems")
	}
}

func structBase() {
	type Employee struct {
		ID            int
		Name, Address string
		DoB           time.time
		Position      string
		Salary        int
		ManagerID     int
	}

	var dilbert Employee
	dilbert.Salary = 5000
	postion := &dilbert.Postion
	*postion = "Sensor " + *postion
	var employeeofTheMonth *Employee = &dilbert
	employeeofTheMonth.Postion += " (proactive team player)"
	id := dilbert.ID
	EmployeeByID(id).Salary = 0
}

func tempBase() {
	const templ = `<p>A: {{.A}}</p><p>B: {{.B}}</p>`
	t := template.Must(template.New("escape").Parse(templ))
	var data struct {
		A string
		B template.HTML
	}
	data.A = "<b>Hello!</b>"
	data.B = "<b>Hello!</b>"
	if err := t.Execute(os.Stdout, data); err != nil {
		log.Fatal(err)
	}
}

func structBase() {
	var w Wheel
	w = Wheel{Circle{Point{8, 8}, 5}, 20}
	w = Wheel{
		Circle: Circle{
			Point:  Point{X: 8, Y: 8},
			Radius: 5,
		},
		Spokes: 20, // NOTE: trailing comma necessary here (and at Radius)
	}
	fmt.Printf("%#v\n", w)

	w.X = 42
	fmt.Printf("%#v\n", w)
}

func jsonBase() {
	{
		data, err := json.Marshal(movies)
		if err != nil {
			log.Fatalf("JSON marshaling failed: %s", err)
		}
		fmt.Printf("%s\n", data)
	}

	{
		data, err := json.MarshalIndent(movies, "", "    ")
		if err != nil {
			log.Fatalf("JSON marshaling failed: %s", err)
		}
		fmt.Printf("%s\n", data)

		var titles []struct{ Title string }
		if err := json.Unmarshal(data, &titles); err != nil {
			log.Fatalf("JSON unmarshaling failed: %s", err)
		}
		fmt.Println(titles) // "[{Casablanca} {Cool Hand Luke} {Bullitt}]"
	}
}

func appendValues(values []int, t *tree) []int {
	if t != nil {
		values = appendValues(values, t.left)
		values = append(values, t.value)
		values = appendValues(values, t.right)
	}
	return values
}

func add(t *tree, value int) *tree {
	if t == nil {
		t = new(tree)
		t.value = value
		return t
	}
	if value < t.value {
		t.left = add(t.left, value)
	} else {
		t.right = add(t.right, value)
	}
	return t
}

func normalSort() {
	var root *tree
	data := make([]int, 50)
	for i := range data {
		data[i] = rand.Int() % 50
	}
	for _, v := range data {
		root = add(root, v)
	}
	appendValues(values[:0], root)
	if !sort.IntsAreSorted(data) {
		t.Errorf("not sorted: %v", data)
	}
}

func main() {
	arrayBase()
	sliceBase()
	mapsBase()
	tempBase()
	structBase()
	jsonBase()
	normalSort()
}
