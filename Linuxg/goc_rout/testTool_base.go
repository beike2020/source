package main

import (
	"bytes"
	"flag"
	"fmt"
	"io"
	"log"
	"math/rand"
	"net/smtp"
	"os"
	"strings"
	"time"
	"unicode"
)

var (
	n               = flag.Bool("n", false, "omit trailing newline")
	s               = flag.String("s", " ", "separator")
	usage           = make(map[string]int64)
	out   io.Writer = os.Stdout // modified during testing
)

const sender = "notifications@example.com"
const password = "correcthorsebatterystaple"
const hostname = "smtp.example.com"
const template = `Warning: you are using %d bytes of storage, %d%% of your quota.`

func echo(newline bool, sep string, args []string) error {
	fmt.Fprint(out, strings.Join(args, sep))
	if newline {
		fmt.Fprintln(out)
	}
	return nil
}

func TestEcho() {
	var tests = []struct {
		newline bool
		sep     string
		args    []string
		want    string
	}{
		{true, "", []string{}, "\n"},
		{false, "", []string{}, ""},
		{true, "\t", []string{"one", "two", "three"}, "one\ttwo\tthree\n"},
		{true, ",", []string{"a", "b", "c"}, "a,b,c\n"},
		{false, ":", []string{"1", "2", "3"}, "1:2:3"},
	}

	for _, test := range tests {
		descr := fmt.Sprintf("echo(%v, %q, %q)", test.newline, test.sep, test.args)

		out = new(bytes.Buffer)
		if err := echo(test.newline, test.sep, test.args); err != nil {
			fmt.Printf("%s failed: %v\n", descr, err)
			continue
		}
		got := out.(*bytes.Buffer).String()
		if got != test.want {
			fmt.Printf("%s = %q, want %q\n", descr, got, test.want)
		}
	}
}

var notifyUser = func(username, msg string) {
	auth := smtp.PlainAuth("", sender, password, hostname)
	err := smtp.SendMail(hostname+":587", auth, sender,
		[]string{username}, []byte(msg))
	if err != nil {
		log.Printf("smtp.SendEmail(%s) failed: %s", username, err)
	}
}

func CheckQuota(username string) {
	used := usage[username]
	const quota = 1000000
	percent := 100 * used / quota
	if percent < 90 {
		return
	}
	msg := fmt.Sprintf(template, used, percent)
	notifyUser(username, msg)
}

func TestCheckQuotaNotifiesUser() {
	var notifiedUser, notifiedMsg string
	const user = "joe@example.org"
	usage[user] = 980000

	notifyUser = func(user, msg string) {
		notifiedUser, notifiedMsg = user, msg
	}

	CheckQuota(user)
	if notifiedUser == "" && notifiedMsg == "" {
		fmt.Println("notifyUser not called")
	}
	if notifiedUser != user {
		fmt.Printf("wrong user (%s) notified, want %s\n", notifiedUser, user)
	}
	const wantSubstring = "98% of your quota"
	if !strings.Contains(notifiedMsg, wantSubstring) {
		fmt.Printf("unexpected notification message <<%s>>, want substring %q\n", notifiedMsg, wantSubstring)
	}
}

func IsPalindrome(s string) bool {
	var letters []rune
	for _, r := range s {
		if unicode.IsLetter(r) {
			letters = append(letters, unicode.ToLower(r))
		}
	}
	for i := range letters {
		if letters[i] != letters[len(letters)-1-i] {
			return false
		}
	}
	return true
}

func randomPalindrome(rng *rand.Rand) string {
	n := rng.Intn(25)
	runes := make([]rune, n)
	for i := 0; i < (n+1)/2; i++ {
		r := rune(rng.Intn(0x1000))
		runes[i] = r
		runes[n-1-i] = r
	}
	return string(runes)
}

func TestIsPalindrome() {
	var tests = []struct {
		input string
		want  bool
	}{
		{"", true},
		{"a", true},
		{"aa", true},
		{"ab", false},
		{"kayak", true},
		{"detartrated", true},
		{"A man, a plan, a canal: Panama", true},
		{"Evil I did dwell; lewd did I live.", true},
		{"Able was I ere I saw Elba", true},
		{"été", true},
		{"Et se resservir, ivresse reste.", true},
		{"palindrome", false}, // non-palindrome
		{"desserts", false},   // semi-palindrome
	}
	for _, test := range tests {
		if got := IsPalindrome(test.input); got != test.want {
			fmt.Printf("IsPalindrome(%q) = %v\n", test.input, got)
		}
	}

	seed := time.Now().UTC().UnixNano()
	rng := rand.New(rand.NewSource(seed))
	for i := 0; i < 1000; i++ {
		p := randomPalindrome(rng)
		if !IsPalindrome(p) {
			fmt.Printf("IsPalindrome(%q) = false\n", p)
		}
	}
}

func main() {
	flag.Parse()
	if err := echo(!*n, *s, flag.Args()); err != nil {
		fmt.Fprintf(os.Stderr, "echo: %v\n", err)
		os.Exit(1)
	}

	TestEcho()
	TestCheckQuotaNotifiesUser()
	TestIsPalindrome()
}
