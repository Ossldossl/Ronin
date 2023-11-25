# The Ronin Programming Language
``` 
CONST_VAL :: 123

main :: fn(x: int, y: int) => int {
	let var1: i32 = x * x;
	let var2 = y * y;
	let result = var1 * var2 + CONST_VAL;
}
``` 

## Syntax
### Deklarationen
Beispiel:
``` 
test1 :: struct {             // Struktur definition
	c_1: int,
	c_2: float,
	c_3: [5]int,
}

test2 :: enum {		          // Aufzählungstyp
	STATUS_ACTIVE,
	STATUS_LOADING,
	STATUS_FINISHED,
}

TEST3 :: 123.45               // Konstante mit inferiertem Typ
TEST4:int : 0xabc123         // Konstante mit Typ-Angabe

foo :: fn(a: u16) => str {...} // Funktions deklaration
```

### Arrays
In Ronin gibt es zwei verschiedene Array-Typen. 
``` 
array_1: []int = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12]
          ^^ Wert muss zur Compile-Zeit bekannt sein (kann aber inferiert werden)
```
Arrays als Parametertyp sehen so aus:
```
main :: fn(array_1: []int, b: int) {
	println(array_1[0..<5])
}
```
Array-Slicing:
```
array_slice := array_1[2..=5]
```
array_slice entspricht hier [2, 3, 4, 5]

### Funktionen
Funktionen in Ronin können mehrere Werte zurückgeben. Die Typen der Rückgabewerte werden in Klammern angegeben. 
```
main :: fn(a: int, b: int) => (int, bool) {...}
                               ^^^^^^^^^ Funktion gibt einen int und einen bool zurück
```
Wenn die Funktion keinen Wert zurückgibt:
```
main :: fn(foo: int, bar: int) {...}
                              ^ diese Funktion gibt nichts zurück
```
Wenn es nur einen Rückgabewert gibt, dann dürfen die Klammern weggelassen werden.
```
main :: fn(argv: [..]String) => int {...}
							    ^^^ Funktion gibt einen int zurück
```
Beim Aufruf von Funktionen kann der Parametername angegeben werden. 
Wenn es Standartwerte für Funktionsparameter gibt, dann dürfen diese im Aufruf weggelassen werden. Default-Werte werden direkt neben den Parametert definitert.
Bsp:
```
foo :: fn(a: int, b: int) {...}

make_point :: fn(x: f32 = 0.f, y: f32 = 0.f) => Point {...}

main :: fn() => () {
	foo(34, 35);
	let point = make_point(x = 32); 
	// 				            ^^ y kann weggelassen werden, da ein Defaultwert feststeht
}
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

main :: fn() => () {
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

main :: fn() => () {
	res: Result
	res.x = 34
	   ^^ ------ Werte innerhalb des Unions sind im Äußeren Scope
}
```
Strukturinitialisierungen:
```
Vec2 :: struct {
	x: int,
	y: int,
}

main :: fn() => () {
	result: Vec2 = {x = 34, y = 35 }
	                ^^^^^^--^^^^^^ ---- x und y werden Initialisiert
}
```
### Enumerationen
Eine normaler Enumerationstyp sieht so aus:
```
GameState :: enum {
	MainMenu,
	Paused,
	Loading,
	Running,
}
```
Enumerationen können Tagged Unions sein und so Werte mit sich verbunden haben.
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
### Traits
Trait definition:
```
Add :: trait {
	add :: fn(self, other: Self) => Self;
}
```
Trait implementierung: 
```
Vector impl Add<i32> {
	add :: fn(self, other: T) => T {
		// ...
	}
}
```
#### Trait als Begrenzungen für generische Typen
Bei Funktions-, Strukturen-, und Enumdefinitionen kann man Beschränkungen für generische Parameter angeben. (=> Typen müssen einen oder mehrere Traits implementieren)
```
foo<T> :: fn(bar: []T) => []T 
	where T: Trait1 + Trait2;
{
	// ...
}

// Beide Varianten möglich
bar<U: Trait1 + Trait2, V: Trait3 = DefaultValue> :: struct 
{
	a1: U,
	a2: V,
} 
```

### Generische Typen
Bei Funktionen:
```
sum<T> :: fn(a: T, b: T) => T {...}

main :: fn(args: []str) => i32{
	sum<i32>(34, 35)
	//  ^^^ kann inferiert werden
}
```

Bei Strukturen:
```
foo<T = i32, U> :: struct {
//    ^^^^^ Default-Wert, d.h. kann im aufruf auch weggelassen werden	
	a: T,
	b: U,
}

main :: fn(..) {
	let test: foo<i32, i64>                                
	let test1 = foo<i32, string> {.a = 34, .b = "35"} 
}
```
Bei Traits
```
Add<Rhs = Self> :: trait {
	add :: fn(self, other: Rhs) => Self 
}

impl Add<Str> for Str {
	add :: fn(self, other: Rhs) => Self {

	}
}

Str impl Add<Str> {
	add :: fn(self, other: Rhs) => Self {
		// Füge beide strings zusammen
	}
}
```

## Grundtypen
```
// ints
i8 i16 i32 i64 i128
u8 u16 u32 u64 u128

// floats
f16 f32 f64

// strings
Str Cstr

bool
```


