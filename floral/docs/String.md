#  String
# WARNING: SPECIFICATION CHANGED

The Floral structure representing strings of characters is encapsulated by `String`. Similarly to `Array` it provides a random-access mutable interface for contiguous characters.

Later in this document the bridged C API will be discussed.

`String` is 24-bytes, composed of three 8-byte unsigned integers. The element of a string is a wide, utf32 character.

#### Member functions

**Concatenation**

```
var a = String("Hello, ");
a.cat("world!");
```

Use the `cat` member function to perform concatenation with another string.

**Appending**
```
var str = String("ab");
str.push('c');
```

Use the `push` member function to append a character to the end of the string.

**Insertion**

```
var carroll = String("'Tut, tut, child!'  'Everything’s got a moral, if only you can find it.'");
carroll.insert("said the Duchess.", 19);
```

All these functions will reallocate the `String` by a growth factor of 1.5 (close to the golden ratio for reusing memory).

**Removal**

```
var verne = String("If there were no thunder, men would oh this shouldn't be here have little fear of lightning.1");
verne.remove(37, 62); // "oh this shouldn't be here"
verne.pop(); // '1'
```

There are two main removal functions: one deletes a whole segment specified by an upper and lower bound, while the other pops off the last character. Neither deallocates any memory. A specific call to `compact` can do so.

**Copying**

```
let str = String("Imitation is the sincerest form of flattery");
let str2 = str.copy();
```

This function returns a new string with the same contents, length and capacity.

**Random-Access**

```
let wonder = String("World keep on turnin'\nCause it won't be too long");
let first = wonder.front(); // type = Char
let last = wonder.end(); // type = Char
let singleQuote = wonder.at(20); // type = Char
let slice = wonder.slice(6, 20); // type = StringView
```

#### C API

With the `using C;` directive (or the `--use-C` compiler flag) you have access to various bridged C functions. These include some string functions. If you wish to perform low-level string interactions, you can use these.

They include:
```
strcpy(dest: &Char, src: &Char): &Char;
strncpy(dest: &Char, src: &Char, n: UInt): &Char;

strcmp(s1: &Char, s2: &Char): Int;
strncmp(s1: &Char, s2: &Char, n: UInt): Int;

strdup(s: &Char): &Char;
strndup(string: &Char, n: UInt): &Char;
```
...and many more.

These functions perform as you would expect them to in C.

You can initialize `String`s with C strings. This will calculate length and convert the ascii into utf32.
