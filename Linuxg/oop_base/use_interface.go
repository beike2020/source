package main

import "fmt"

type ISpeaker interface {
	Speak()
}

type SimpleSpeaker struct {
	Message string
}

func (speaker *SimpleSpeaker) Speak() {
	fmt.Println("I am speaking ...... ", speaker.Message)
}

func main() {
	var speaker ISpeaker
	speaker = &SimpleSpeaker{"Hello"}
	if _, ok := speaker.(ISpeaker); ok {
		fmt.Println("speaker have Speaker func")
	}
	speaker.Speak()
}
