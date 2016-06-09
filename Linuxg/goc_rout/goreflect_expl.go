package main

import (
	"bytes"
	"fmt"
	"reflect"
	"strconv"
	"text/scanner"
)

const margin = 80

type lexer struct {
	scan  scanner.Scanner
	token rune
}

type token struct {
	kind rune
	str  string
	size int
}

type printer struct {
	tokens []*token
	stack  []*token
	rtotal int

	bytes.Buffer
	indents []int
	width   int
}

func encode(buf *bytes.Buffer, v reflect.Value) error {
	switch v.Kind() {
	case reflect.Invalid:
		buf.WriteString("nil")

	case reflect.Int, reflect.Int8, reflect.Int16,
		reflect.Int32, reflect.Int64:
		fmt.Fprintf(buf, "%d", v.Int())

	case reflect.Uint, reflect.Uint8, reflect.Uint16,
		reflect.Uint32, reflect.Uint64, reflect.Uintptr:
		fmt.Fprintf(buf, "%d", v.Uint())

	case reflect.String:
		fmt.Fprintf(buf, "%q", v.String())

	case reflect.Ptr:
		return encode(buf, v.Elem())

	case reflect.Array, reflect.Slice:
		buf.WriteByte('(')
		for i := 0; i < v.Len(); i++ {
			if i > 0 {
				buf.WriteByte(' ')
			}
			if err := encode(buf, v.Index(i)); err != nil {
				return err
			}
		}
		buf.WriteByte(')')

	case reflect.Struct:
		buf.WriteByte('(')
		for i := 0; i < v.NumField(); i++ {
			if i > 0 {
				buf.WriteByte(' ')
			}
			fmt.Fprintf(buf, "(%s ", v.Type().Field(i).Name)
			if err := encode(buf, v.Field(i)); err != nil {
				return err
			}
			buf.WriteByte(')')
		}
		buf.WriteByte(')')

	case reflect.Map:
		buf.WriteByte('(')
		for i, key := range v.MapKeys() {
			if i > 0 {
				buf.WriteByte(' ')
			}
			buf.WriteByte('(')
			if err := encode(buf, key); err != nil {
				return err
			}
			buf.WriteByte(' ')
			if err := encode(buf, v.MapIndex(key)); err != nil {
				return err
			}
			buf.WriteByte(')')
		}
		buf.WriteByte(')')

	default:
		return fmt.Errorf("unsupported type: %s", v.Type())
	}
	return nil
}

func Marshal(v interface{}) ([]byte, error) {
	var buf bytes.Buffer
	if err := encode(&buf, reflect.ValueOf(v)); err != nil {
		return nil, err
	}
	return buf.Bytes(), nil
}

func (lex *lexer) next() {
	lex.token = lex.scan.Scan()
}

func (lex *lexer) text() string {
	return lex.scan.TokenText()
}

func (lex *lexer) consume(want rune) {
	if lex.token != want {
		panic(fmt.Sprintf("got %q, want %q", lex.text(), want))
	}
	lex.next()
}

func read(lex *lexer, v reflect.Value) {
	switch lex.token {
	case scanner.Ident:
		if lex.text() == "nil" {
			v.Set(reflect.Zero(v.Type()))
			lex.next()
			return
		}
	case scanner.String:
		s, _ := strconv.Unquote(lex.text())
		v.SetString(s)
		lex.next()
		return
	case scanner.Int:
		i, _ := strconv.Atoi(lex.text())
		v.SetInt(int64(i))
		lex.next()
		return
	case '(':
		lex.next()
		readList(lex, v)
		lex.next()
		return
	}
	panic(fmt.Sprintf("unexpected token %q", lex.text()))
}

func endList(lex *lexer) bool {
	switch lex.token {
	case scanner.EOF:
		panic("end of file")
	case ')':
		return true
	}
	return false
}

func readList(lex *lexer, v reflect.Value) {
	switch v.Kind() {
	case reflect.Array:
		for i := 0; !endList(lex); i++ {
			read(lex, v.Index(i))
		}

	case reflect.Slice:
		for !endList(lex) {
			item := reflect.New(v.Type().Elem()).Elem()
			read(lex, item)
			v.Set(reflect.Append(v, item))
		}

	case reflect.Struct:
		for !endList(lex) {
			lex.consume('(')
			if lex.token != scanner.Ident {
				panic(fmt.Sprintf("got token %q, want field name", lex.text()))
			}
			name := lex.text()
			lex.next()
			read(lex, v.FieldByName(name))
			lex.consume(')')
		}

	case reflect.Map:
		v.Set(reflect.MakeMap(v.Type()))
		for !endList(lex) {
			lex.consume('(')
			key := reflect.New(v.Type().Key()).Elem()
			read(lex, key)
			value := reflect.New(v.Type().Elem()).Elem()
			read(lex, value)
			v.SetMapIndex(key, value)
			lex.consume(')')
		}

	default:
		panic(fmt.Sprintf("cannot decode list into %v", v.Type()))
	}
}

func Unmarshal(data []byte, out interface{}) (err error) {
	lex := &lexer{scan: scanner.Scanner{Mode: scanner.GoTokens}}
	lex.scan.Init(bytes.NewReader(data))
	lex.next()
	defer func() {
		if x := recover(); x != nil {
			err = fmt.Errorf("error at %s: %v", lex.scan.Position, x)
		}
	}()
	read(lex, reflect.ValueOf(out).Elem())
	return nil
}

func (p *printer) string(str string) {
	tok := &token{kind: 's', str: str, size: len(str)}
	if len(p.stack) == 0 {
		p.print(tok)
	} else {
		p.tokens = append(p.tokens, tok)
		p.rtotal += len(str)
	}
}

func (p *printer) pop() (top *token) {
	last := len(p.stack) - 1
	top, p.stack = p.stack[last], p.stack[:last]
	return
}

func (p *printer) begin() {
	if len(p.stack) == 0 {
		p.rtotal = 1
	}
	t := &token{kind: '(', size: -p.rtotal}
	p.tokens = append(p.tokens, t)
	p.stack = append(p.stack, t) // push
	p.string("(")
}

func (p *printer) end() {
	p.string(")")
	p.tokens = append(p.tokens, &token{kind: ')'})
	x := p.pop()
	x.size += p.rtotal
	if x.kind == ' ' {
		p.pop().size += p.rtotal
	}
	if len(p.stack) == 0 {
		for _, tok := range p.tokens {
			p.print(tok)
		}
		p.tokens = nil
	}
}

func (p *printer) space() {
	last := len(p.stack) - 1
	x := p.stack[last]
	if x.kind == ' ' {
		x.size += p.rtotal
		p.stack = p.stack[:last] // pop
	}
	t := &token{kind: ' ', size: -p.rtotal}
	p.tokens = append(p.tokens, t)
	p.stack = append(p.stack, t)
	p.rtotal++
}

func (p *printer) print(t *token) {
	switch t.kind {
	case 's':
		p.WriteString(t.str)
		p.width -= len(t.str)
	case '(':
		p.indents = append(p.indents, p.width)
	case ')':
		p.indents = p.indents[:len(p.indents)-1] // pop
	case ' ':
		if t.size > p.width {
			p.width = p.indents[len(p.indents)-1] - 1
			fmt.Fprintf(&p.Buffer, "\n%*s", margin-p.width, "")
		} else {
			p.WriteByte(' ')
			p.width--
		}
	}
}

func (p *printer) stringf(format string, args ...interface{}) {
	p.string(fmt.Sprintf(format, args...))
}

func pretty(p *printer, v reflect.Value) error {
	switch v.Kind() {
	case reflect.Invalid:
		p.string("nil")

	case reflect.Int, reflect.Int8, reflect.Int16,
		reflect.Int32, reflect.Int64:
		p.stringf("%d", v.Int())

	case reflect.Uint, reflect.Uint8, reflect.Uint16,
		reflect.Uint32, reflect.Uint64, reflect.Uintptr:
		p.stringf("%d", v.Uint())

	case reflect.String:
		p.stringf("%q", v.String())

	case reflect.Array, reflect.Slice:
		p.begin()
		for i := 0; i < v.Len(); i++ {
			if i > 0 {
				p.space()
			}
			if err := pretty(p, v.Index(i)); err != nil {
				return err
			}
		}
		p.end()

	case reflect.Struct:
		p.begin()
		for i := 0; i < v.NumField(); i++ {
			if i > 0 {
				p.space()
			}
			p.begin()
			p.string(v.Type().Field(i).Name)
			p.space()
			if err := pretty(p, v.Field(i)); err != nil {
				return err
			}
			p.end()
		}
		p.end()

	case reflect.Map:
		p.begin()
		for i, key := range v.MapKeys() {
			if i > 0 {
				p.space()
			}
			p.begin()
			if err := pretty(p, key); err != nil {
				return err
			}
			p.space()
			if err := pretty(p, v.MapIndex(key)); err != nil {
				return err
			}
			p.end()
		}
		p.end()

	case reflect.Ptr:
		return pretty(p, v.Elem())

	default:
		return fmt.Errorf("unsupported type: %s", v.Type())
	}
	return nil
}

func MarshalIndent(v interface{}) ([]byte, error) {
	p := printer{width: margin}
	if err := pretty(&p, reflect.ValueOf(v)); err != nil {
		return nil, err
	}
	return p.Bytes(), nil
}

func main() {
	type Movie struct {
		Title, Subtitle string
		Year            int
		Actor           map[string]string
		Oscars          []string
		Sequel          *string
	}

	strangelove := Movie{
		Title:    "Dr. Strangelove",
		Subtitle: "How I Learned to Stop Worrying and Love the Bomb",
		Year:     1964,
		Actor: map[string]string{
			"Dr. Strangelove":            "Peter Sellers",
			"Grp. Capt. Lionel Mandrake": "Peter Sellers",
			"Pres. Merkin Muffley":       "Peter Sellers",
			"Gen. Buck Turgidson":        "George C. Scott",
			"Brig. Gen. Jack D. Ripper":  "Sterling Hayden",
			`Maj. T.J. "King" Kong`:      "Slim Pickens",
		},
		Oscars: []string{
			"Best Actor (Nomin.)",
			"Best Adapted Screenplay (Nomin.)",
			"Best Director (Nomin.)",
			"Best Picture (Nomin.)",
		},
	}

	data, err := Marshal(strangelove)
	if err != nil {
		fmt.Printf("Marshal failed: %v\n", err)
	}
	fmt.Printf("Marshal() = %s\n", data)

	var movie Movie
	if err := Unmarshal(data, &movie); err != nil {
		fmt.Printf("Unmarshal failed: %v\n", err)
	}
	fmt.Printf("Unmarshal() = %+v\n", movie)

	if !reflect.DeepEqual(movie, strangelove) {
		fmt.Println("not equal")
	}

	data, err = MarshalIndent(strangelove)
	if err != nil {
		fmt.Println(err)
	}
	fmt.Printf("MarshalIdent() = %s\n", data)
}
