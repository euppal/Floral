#  Structs

Alongside the built-in types, Floral allows the programmer to define their own types through `struct`s. A basic example follows:

```
// A primitive struct
struct Foo {
    let w: Int;
    let x: String;
    let y: Bool
    let z: &Float;
};
```

`struct`s are aggregations of built-in data types and/or other `struct`s as well as functions. These are called data and function members respectively. Static allocation is as simple as the `struct` name along with it's members as arguments.

```
let float: Float = 3.14;
let foo = Foo(3, "Hello", false, &float);
```

Member access is through `.` and this can be chained across `struct`s:

```
print(foo.bar.baz, '\n');
```

Internally, `struct`s are statically allocated on the stack, in member-wise order, with offsets based on the size of data members. Hence, the instance of `Foo` above would be layed out in memory as:

```
+--------+--------+--------+--------+
| rbp-16 | rbp-24 | rbp-32 | rbp-40 |
| 0x3    | Hello  | false  | 0x7f.. |
+--------+--------+--------+--------+
-> stack grows downwards
```

`struct`s also allow you to define function members.

```
struct Counter {
    Counter(): value = 0;

    func inc() {
        this->value++;
    }

public get:
    var value: Int;
};
```

There are a couple new things here. One is the initializer which, similarly to C++, is defined with the name of the `struct` alongside any arguments. Following the colon you can initialise any members, or you can omit the colon altogether. After, a statement is expected. This statement can be a block where calculations can be made or data members can be dynamically allocated.

Alongside the initializer, with the deinitializer `~TypeName() {}` you can perform necessary cleanup and free dynamically allocated memory.

There is also the struct section `public get` which is composed of two keywords. A section declared `public` or `public get set` will have its members visible and externally mutable (`public get set` is the default). A section  declared `public get` will be externally visible and immutable. There is also `private` which declares a section only accessible from within the `struct`.

Here is an example:

```
struct Sections {
// public [get set]:
    var open: Int;

private:
    var secret: Int; // only function members can access this

public get:
    var getter: Int; // can be read but not set
};
```

Let's take a closer look at the member function of our counter:

```
struct Counter {
...
    func inc() {
        self->value++;
    }
...
};
```

The identifier `self` is a special identifier which is a pointer to the `struct` a being called. You can use it to reference and mutate the data members of the `struct`. In this case, it's used to increment the `Counter`s value.

Member functions are called similarly to how data members are accessed.

```
var counter = Counter();
counter.inc(); // counter.value == 1
counter.inc(); // counter.value == 2
print(counter.value, '\n');
```

