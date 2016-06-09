package main

import "fmt"

type PersonInfo struct {
	ID      string
	Name    string
	Address string
}

func map_struct() {
	var personDB map[string]PersonInfo
	personDB = make(map[string]PersonInfo, 10)

	personDB["1"] = PersonInfo{"1", "Jack", "Room 101,..."}
	personDB["12345"] = PersonInfo{"12345", "Tom", "Room 203,..."}
	for key, value := range personDB {
		fmt.Printf("Key=%s, Value=%s\n", key, value)
	}

	fmt.Println("Delete Key: 12345")
	delete(personDB, "12345")

	person, ok := personDB["12345"]
	if ok {
		fmt.Println("Found person", person.Name, "with ID 12345.")
	} else {
		fmt.Println("Did not find person with ID 12345.")
	}

	fmt.Println()
}

func array_struct() {
	var myArray [10]int = [10]int{1, 2, 3, 4, 5, 6, 7, 8, 9, 10}

	mySlice := make([]int, 5, 10)
	fmt.Printf("len(mySlice): %d, cap(mySlice): %d.\n", len(mySlice), cap(mySlice))

	mySlice = myArray[:5]
	fmt.Println("Elements of myArray: ")
	for v := range myArray {
		fmt.Print(v, " ")
	}

	fmt.Println("\nElements of mySlice: ")
	for i, v := range mySlice {
		fmt.Print(i, ":", v, ", ")
	}
	fmt.Println()
	fmt.Println()
}

func main() {
	array_struct()
	map_struct()
}
