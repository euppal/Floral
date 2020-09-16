#  Structs

Alongside the built-in types, Floral allows the programmer to define their own types through `struct`s. A basic example follows:

```
// A primitive struct
struct Foo {
    let x: Int;
    var y: String;
    let z: &Float;
};
```

`struct`s are aggregations of built-in data types and/or other `struct`s as well as functions. These are called data and function members respectively. Static allocation is as simple as the `struct` name along with it's members as arguments.

```
let float: Float = 3.14;
let foo = Foo(3, "Hello", &float);
```

Member access is through `.` and this can be chained across `struct`s:

```
print(foo.bar.baz, '\n');
```

Internally, `struct`s are statically allocated on the stack, in member-wise order, with offsets based on the size of data members. Hence, initialization of `Foo` above would involve (ex: `let foo = Foo(3, "Hello", ptrToSomeFloat);`):

```
...
_floralid_main:
  ...
  sub rsp, 40; alignof(Foo) + alignof(Float)
  mov [rbp-8], 0x3FB33333 ; The hex is the binary representation of 3.14
  mov [rbp-8], 3; 
  mov [rbp-16], lit0_str
  mov [rbp-24], lit0_str.len
  mov [rbp-32], rbp-8
  ...
  add rsp, 40 ;
  ...

section .rodata
  strlit0: db `Hello`
  .len: equ $ - lit2_str
```

`struct`s also allow you to define function members.

```
struct Counter {
    Counter(): value = 0;

    func inc() {
        this->value++;
    }

exposed get:
    var value: Int;
};
```

There are a couple new things here. One is the initializer which, similarly to C++, is defined with the name of the `struct` alongside any arguments. Following the colon you can initialise any members, or you can omit the colon altogether. After, a statement is expected. This statement can be a block where calculations can be made or data members can be dynamically allocated.

Alongside the initializer, with the deinitializer `~TypeName() {}` you can perform necessary cleanup and free dynamically allocated memory.

There is also the struct section `exposed get` which is composed of two keywords. A section declared `exposed` or `exposed get set` will have its members visible and externally mutable (`exposed get set` is the default). A section  declared `exposed get` will be externally visible and immutable. There is also `internal` which declares a section only accessible from within the `struct`.

Here is an example:

```
struct Sections {
// exposed [get set]:
    var open: Int;

internal:
    var secret: Int; // only function members can access this

exposed get:
    var getter: Int; // can be read but not set
};
```

Let's take a closer look at the member function of our counter:

```
struct Counter {
...
    func inc() {
        this->value++;
    }
...
};
```

The identifier `this` is a special identifier which is a pointer to the `struct` a being called. You can use it to reference and mutate the data members of the `struct`. In this case, it's used to increment the `Counter`s value.

Member functions are called similarly to how data members are accessed.

```
var counter = Counter();
counter.inc(); // counter.value == 1
counter.inc(); // counter.value == 2
print(counter.value, '\n');
```

