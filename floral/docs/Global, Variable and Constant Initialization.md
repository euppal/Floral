#  Variables, Constants and Initialization

The chief format for storing, passing around and operating on data involves the declaration of variables or constants and then their initialization. Here is an example Floral program:

```
func main(): Int {
    var x = 1;
    var y = 2;
    var z = x+y;
    return 0;
}
```

In this example, the variable `x` is declared and initialized to the integer `1`, the variable `y` is declared and initialized to the integer `2` and `z` is declared and initialized to the sum of `x` and `y` (`1+2`). 

Variables, however, are just one way of operating on data. Floral provides three: global constants, local constants and local variables. In the example above, 'x', 'y' and 'z' are all local variables due to them being declared by the keyword `var`. Global constants are declared with the keyword `global` and local constants are declared with the keyword `let`. 

Global constants and string literals do not exist within the process's text section. Rather, depending on the type of initialization, they exist within read-only data or zero-initialized data sections.

Take for example the following code:

```
global x: Int();
```

Empty parenthesis `()` indicate zero-initialization. To zero-initialize, you must provide a type (in this case, 64-bit `Int`). There is no whitespace requirement, so `global x: Int ();` is the same as `global x: Int();`, however the latter syntax is preferred. 

When an expression is provided within the parentheses, direct-initialization occurs. This means that the expression is statically evaluated and placed in the read-only data section of the process. For local variables and constants, direct-initialization simple performs copy-initialization. 

Using an equal sign (`=`) after the declaration indicates copy-initialization. Copy-initializing allocates space for the type on the stack and afterwards initializes. For global constants, copy-initialization simply performs direct-initialization.

Let's view an example:

```
global x: Int();
global y: Int(1);
global z: Int = 2;
```
...this gets compiled to:
```nasm
section .text
global _main
_main:
  mov rax, 0x2000001
  xor rdi, rdi
  syscall

section .rodata
  y: dq 1
  z: dq 2

section .bss
  x: resq 1
```

Since `x` was zero-initialized to `Int`, 64-bits of zeros are reserved (see `x: resq 1` which reserves one quad-word, or four 16-bit words). Both `y` and `z` are direct-initialized in `section .rodata` because even though `global z` was declared with the copy-initialization syntax, global constants cannot be modified after their definition, so the compiler simply direct-initializes them.
