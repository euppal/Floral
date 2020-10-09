# Floral Standard Library
# WARNING: SPECIFICATION CHANGED

The Floral standard library defines a multitude of functions to help programmers build functional programs. This document describes the standard library in detail.

> By default, Floral compiles with its standard library. You can turn this off with the flag `--no-stdlib-header`, in which case `using stdlib;` is necessary to use its functionality.

#### How entries are formatted

For functions, the forward declaration will be provided, then the parameters with explanations and finally a description. For types, the the forward declaration will be provided, then the members with explanations and finally a description.

##### functionEx
```
functionEx(arg1: Type1, arg2: Type2);
```
---
Parameter | Description
--------- | -----------
**arg1** | An example argument  
**arg2** | Another example argument

This function performs the following actions.

##### ExampleStruct
```
predeclare struct ExampleStruct {
    var dataMember: Int;
    func functionMember(): Bool;
};
```
---
Member | Description  
------ | -----------
**dataMember** | A data member
**functionMember(): Bool** | A function member returning a boolean

This `struct` is being used for example purposes.

#### Standard Library Functions

##### print(String) _[I/O]_
```
func print(str: String);
```
---
Parameter | Description
--------- | -----------
**str** | The string to be printed

The `print` function writes the specified string to the standard output.

##### print\<T>(T, Char) _[I/O]_
```
print<T: Describable>(value: T, term: Char);
```
---
Parameter | Description
--------- | -----------
**value** | The value to be printed, assuming it adheres to the `Describable` behavior  
**term** | The terminator character, or `'\0'` if no terminator is desired

The `print<T>` function prints any describable value alongside a terminator.

##### fprint(str: String, file: &File) _[File I/O]_

Parameter | Description
--------- | -----------
**str** | The string to be printed  
**file** | The file pointer of the destination

The `fprint` function writes the specified string to the provided file.

##### read(UInt): String _[I/O]_
```
func read(bytes: UInt): String;
```
---
Parameter | Description
--------- | -----------
**bytes** | The amount of bytes to be read

The `read` function extracts the specified number of bytes from the standard input and returns a dynamically allocated string of those bytes.

##### fread(UInt, &File): String _[File I/O]_
---
```
func fread(bytes: UInt, file: &File): String;
```
---
Parameter | Description
--------- | -----------
**bytes** | The amount of bytes to be read
**file** | The file to be read from

The `fread` function extracts the specified number of bytes from the given file and returns a dynamically allocated string of those bytes.

##### alloc<T>(Type, UInt): &T _[Memory Management]_
---
```
func alloc<T>(Type, n: UInt): &T;
```
---
Parameter | Description
--------- | -----------
**type** | A type literal indicating what to allocate
**n** | The amount of the specified type to allocate

The `alloc` function allocates memory and returns a pointer to it.

##### dealloc<T>(&T) _[Memory Management]_
---
```
func dealloc<T>(ptr: &T);
```
---
Parameter | Description
--------- | -----------
**ptr** | The pointer returned by the corresponding call to `alloc`.


The `dealloc` function frees the memory previously allocated.

#### Standard Library Types

##### _struct_ String
```
predeclare struct String {
    String();
    String(cstring: &Char);
    ~String();
    var start: &Char;
    var length: UInt;
    var capacity: UInt;
    func cat(str: String);
    func push(c: Char);
    func insert(str: String, loc: UInt);
    func remove(s: UInt, e: UInt);
    func pop();
    func compact();
    func copy(): String;
    func cstr(): &Char;
    func front(): Char;
    func end(): Char;
    func at(i: UInt): Char;
    func slice(s: UInt, e: UInt): StringView;
};
```
---
Member | Description
------ | -----------
**String()** | Initializes an empty string with capacity = 4  
**String(&Char)** | Initializes the string with the specified cstring, length counting null terminator. If necessary, you can `pop()` the null terminator afterwards  
**~String()** | Deallocates the string if dynamically allocated  
**start** | A pointer to the start of the string  
**length** | The length of the string  
**capacity** | The capacity of the string  
**cat(String)** | Concatenates the given string to the current string  
**push(Char)** | Pushes a character to the back of the string  
**insert(String, UInt)** | Inserts the given string at the specified location  
**remove(UInt, UInt)** | Removes the specified section of the string  
**pop()** | Pops off the last character of the string  
**compact()** | Reallocates to have just enough capacity >= 4 for the string  
**copy(): String** | Returns a string copy with contents identical to the original  
**cstr(): &Char** | Returns a cstring copy with contents identical to the original, save the null terminator However, if the string was initialized as a cstring and has not been modified, no copying is performed  
**front(): Char** | Returns the character at the front of the string
**back(): Char** | Returns the character at the back of the string, including a null terminator
**at(UInt): Char** | Returns the character at the specified index
**slice(UInt, UInt): StringView** | Returns a window into a slice of the string. Deallocating the original string invalidates this window

`struct String` provides an interface to variable length strings or string constants. It can represent both dynamic strings and string constants (in which case `capacity == 0`. If a string constant is to be mutated, it is converted to a dynamically allocated string with just enough memory to store it. Otherwise, the starting capacity is 4 and it grows by a factor of 1.5.

##### _struct_ StringView
```
predeclare struct StringView {
    var start: &Char;
    var length: UInt
    func front(): Char;
    func end(): Char;
    func at(i: UInt): Char;
    func slice(s: UInt, e: UInt): StringView;
};
```
---
Member | Description
------ | -----------
**start** | A pointer to the start of the string view
**length** | The length of the string view
**front(): Char** | Returns the character at the front of the string view
**back(): Char** | Returns the character at the back of the string view, including a null terminator
**at(UInt): Char** | Returns the character at the specified index
**slice(UInt, UInt): StringView** | Returns a window into a slice of the original string. Deallocating the original string invalidates this window

`struct StringView` provides a window into a string without any extraneous allocation, meaning slicing a string is very cheap.

##### _struct_ Array
```
predeclare struct Array<E> {
    Array();
    Array(ptr: &E, length: UInt);
    ~Array();
    var start: &E;
    var length: UInt;
    var capacity: UInt;
    func append(a: Array<E>);
    func push(e: E);
    func insert(a: Array<E>, loc: UInt);
    func remove(s: UInt, e: UInt);
    func pop();
    func compact();
    func copy(): Array<E>;
    func front(): E;
    func end(): E;
    func at(i: UInt): E;
    func slice(s: UInt, e: UInt): Slice<E>;
};
```
---
Member | Description
------ | -----------
**Array()** | Initializes an empty string with capacity = 0  
**Array(&E, UInt)** | Initializes the array with a pointer
**~Array()** | Deallocates the array
**start** | A pointer to the start of the array
**length** | The length of the array  
**capacity** | The capacity of the array
**append(a: Array<E>)** | Appends the given array to the array
**push(E)** | Pushes an element to the back of the array  
**insert(Array<E>, UInt)** | Inserts the given array at the specified location  
**remove(UInt, UInt)** | Removes the specified section of the array  
**pop()** | Pops off the last element of the array 
**compact()** | Reallocates to have just enough capacity for the array  
**copy(): Array<E>** | Returns a new array with contents identical to the original
**front(): E** | Returns the first element of the array
**back(): E** | Returns the last element of the array
**at(UInt): E** | Returns the element at the specified index
**slice(UInt, UInt): Slice<E>** | Returns a window into a slice of the array. Deallocating the original array invalidates this window

##### _struct_ Slice
```
predeclare struct Slice<E> {
    var start: &E;
    var length: UInt
    func front(): E;
    func end(): E;
    func at(i: UInt): E;
    func slice(s: UInt, e: UInt): Slice<E>;
};
```
---
Member | Description
------ | -----------
**start** | A pointer to the start of the array slice
**length** | The length of the array slice
**front(): E** | Returns the character at the front of the array slice
**back(): E** | Returns the character at the back of the string view, including a null terminator
**at(UInt): E** | Returns the element at the specified index
**slice(UInt, UInt): Slice<E>** | Returns a window into a slice of the original string. Deallocating the original string invalidates this window

`struct Slice` provides a window into an array without any extraneous allocation, meaning slicing a array is very cheap.
