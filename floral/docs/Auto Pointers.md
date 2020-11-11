# Automatic Pointers

By default, Floral uses manual memory managment. This means that it is the responsibility of the programmer to make sure memory is used safely. However, this style of programming is also prone to errors such as dangling pointers and the like. As such, Floral also provides automatic semantics for pointer management and deallocation. 

Automatic (or simply "auto") pointers are declared with an `auto` in the let or var statement:

```
let auto ptr = alloc(sizeof(Char), 13) :: &Char;
```

Then, at the end of the function, `ptr` is automatically deallocated (i.e. the compiler inserts `dealloc(ptr)`).

What if we pass `ptr` to another function, like `sayHello`?

```
#include <io.fh>
#include <cbridge.fh>

func sayHello(str: &const Char) {
   print("Hello, {s}!\n", str);
}

func main(): Int {
    let auto ptr = alloc(sizeof(Char), 13) :: &Char;
    strncpy(ptr, "Hello, world", 13);
    sayHello(ptr);
    sayHello(ptr);
    return 0;
}
```

This code is fine up until the second `sayHello`. Why? When you pass an auto pointer as an argument the function _takes ownership of the pointer_. From then on, the function owns the pointer and thus has the responsibility of deallocating it. So by the time the second `sayHello` is reached, `ptr` is dangling. 

So what is the solution for this? Well, you can mark a parameter as `unowned` like so:

```
func sayHello(str: unowned &const Char) {
   print("Hello, {s}!\n", str);
}
```

> You might notice that `strncpy` seems to take ownership of `ptr` before the `sayHello` calls. You would be right, however `strncpy` marks its parameters with `unowned`.

This way the `sayHello` function does **not** take ownership of `ptr` and the code compiles. With this keyword, `ptr` gets deallocated at the end of `main`.
