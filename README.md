# The Ronin Programming Language
``` code
CONST_VAL :: 123

main :: fn(x: int, y: int) -> int {
	var1: i32 = x * x
	var2 := y * y
	result = var1 * var2 + CONST_VAL
}
``` 

## Syntax
### Deklarationen
Beispiel:
``` odin
test1 :: struct {             // Struktur definition
	c_1: int,
	c_2: float,
	c_3: [..]int,
}

test2 :: enum {		          // Aufzählungstyp
	STATUS_ACTIVE,
	STATUS_LOADING,
	STATUS_FINISHED
}

TEST3 :: 123.45               // Konstante mit inferiertem Typ
TEST4 :int : 0xabc123         // Konstante mit Typ-Angabe

foo :: fn() -> string {...} // Funktions deklaration
```

### Arrays
In Ronin gibt es zwei verschiedene Array-Typen. "Normale" Arrays sind, im Gegensatz zu C Arrays, die eigentlich nur Pointer sind, ein Pointer + die Größe des Arrays. Die Größe muss zur Compile-Zeit bekannt sein.
``` 
array_1: []int = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12]
          ^^ Wert muss zur Compile-Zeit bekannt sein (kann aber inferiert werden)
```
Arrays als Parametertyp sehen so aus:
```
main :: fn(array_1: []int, b: int) {
	println(array_1[1])
}
```
Dies erlaubt Array-Slicing.
```
array_slice := array_1[2..=5]
```
array_slice entspricht hier [2, 3, 4, 5]

### Funktionen
Funktionen in Ronin können mehrere Werte zurückgeben. Die Typen der Rückgabewerte werden in Klammern angegeben. 
```
main :: fn(a: int, b: int) -> (int, bool) {...}
                                 ^^^^^^^^^ Funktion gibt einen int und einen bool zurück
```
Ein leeres Klammernpaar zeigt an, dass die Funktion keinen Wert zurück gibt (equivalent zu void in C). Der Rückgabetyp kann in diesem Fall auch weggelassen werden.
```
main :: fn(foo: int, bar: int) -> () {...}
									^^ Funktion gibt keinen Wert zurück
main :: fn(foo: int, bar: int) {...}
                                ^^ auch diese Funktion gibt nichts zurück
```
Wenn es nur einen Rückgabewert gibt, dann dürfen die Klammern weggelassen werden.
```
main :: fn(argv: [..]String) -> int {...}
								  ^^^ Funktion gibt einen int zurück
```
Beim Aufruf von Funktionen kann der Parameter name angegeben werden. Bsp:
In einem Funktionsaufruf dürfen genannte und nicht genannte Parameter gemischt werden. Wenn Parameternamen genannt werden, dürfen diese auch in anderer Reihenfolge sein.
```
foo :: fn(a: int, b: int) -> () {...}

bar :: fn(a: int, b: int) 

main :: fn() -> () {
	foo(a: 34, 35)
        ^^^^^^^^^ Parameter a wird genannt, Parameter b nicht

	foo(b: 123, 22)
	    ^^^^^^^^^^ Erzeugt einen Fehler; nicht benannte Parameter müssen in der Reinfolge der Deklaration bleiben 

	foo(b: 123, a: 22)
		^^^^^^^^^^^^^ Alles gut
}
```

Um default-Werte für Funktionsparameter zu definieren, wird das Keyword 'with' benutzt.
```
create_rect :: fn(x: i32, y: i32, color: i32) -> ^Rect 
	with {color = 0xFF00FF}

```

### Strukturen
Strukturen beinhalten 1 oder mehr Werte. Die Werte werden mit Kommas getrennt
```
Vec2 :: struct {
	x: int,
	y: int,
}
```
Strukturen können Unions enthalten.
``` 
Result :: struct {
	kind: int,
	values :: union {
	^^^^^^ ------------ Union hat einen Namen
		x: i32,
		y: i64,
	},
}

main :: fn() -> () {
	res: Result
	res.values.x = 34
	    ^^^^^^ ------- Auf Werte innerhalb des Unions werden über den Union zugegriffen 
}
```
Diese müssen nicht zwingend einen Namen haben. Ist dies der Fall werden die Werte innerhalb des Unions so behandelt, als ob sie normale Member der Struktur wären.
``` 
Result :: struct {
	kind: int,
	union {
	^^^^^ ------- Union hat keinen Namen
		x: i32,
		y: i64,
	},
}

main :: fn() -> () {
	res: Result
	res.x = 34
	   ^^ ------ Werte innerhalb des Unions sind im Äußeren Scope
}
```
Strukturen können Initialisiert werden. Die Werte bestehen aus '.' und ihrem identifier. Der Wert, der den Feldern zugeordnet werden soll, kommt nach einem ':'.
```
Vec2 :: struct {
	x: int,
	y: int,
}

main :: fn() -> () {
	result: Vec2 = {.x: 34, .y: 35 }
	                ^^^^^^  ^^^^^^ ---- x und y werden Initialisiert
}
```
### Enumerationen
Enumerationen in Ronin sind quasi Tagged Unions. Enums können Werte mit sich verbunden haben.
```
Result :: enum {
	Ok(i32),
	Failed(i64),
}
```
Diese Enumeration wird intern in einen Tagged Union "verwandelt".
```
Result :: struct {
	kind: i32,
	union {
		_1: i32, // case: Ok(i32)     
		_2: i64  // case: Failed(i64) 
	}
}
```
### Generische Typen
Bei Funktionen:
```
sum<T> :: fn(a: $T, b: $T)        -> $T {...}

sum :: fn($T: type, a: $T, b: $T) -> $T {...}

main :: fn(..) {
	sum<i32>(34, 35)
	sum(34, 35)

	sum($i32, 34, 35)
}
```

Bei Strukturen:
```
foo<T, U> :: struct {
	a: $T,
	b: $U,
}

foo :: struct($T, $U) {   // inkonsistent mit aufruf :(
	a: $T,
	b: $U,
}

main :: fn(..) {
	let test: foo<i32, i64>                                // ✅
	let test1 = foo<i32, string> {.a = 34, .b = "35"} 

	let test: foo($i32, $i64) // sieht aus wie funktionsaufruf, also nein :(
	let test1 = foo($i32, $string) {.a = 34, .b = "35"} 
}
```

### Traits
// TODO::


#### Trait als Begrenzungen
Bei Funktions-, Strukturen-, und Enumdefinitionen kann man Beschränkungen für generische Parameter angeben. (=> Typen müssen einen oder mehrere Traits implementieren)
```
foo :: trait {
	do_sth :: fn() {...}
}

thing :: struct {
	a: int, b: int
}

impl foo for thing {
	do_sth :: fn() { return 34 + 35}
}

bar :: fn($T: type, b: i32) -> bool
	where $T => foo
```
## Grundtypen
```
// ints
int  i8 i16 i32 i64 i128
uint u8 u16 u32 u64 u128

// floats
f16 f32 f64

rune  // signed 32-bit int
      // steht für einen utf-8 char

// strings
string cstring

bool

// void* equivalent
rawptr

// Typ Informationen
typeid
```
*String*	-> pointer + größe  
*CString* -> String wie in C


