#  Start

This is a guide which describes how one can compile and execute Floral. The requirements to do so are few: you will need a C++ compiler, a nasm compiler, and a linker.

First, write your floral code.

```
func main(): Int {
    print("Hello, world!\n");
    return 0;
}
```

Then, run the compiler with your `.floral` file alongside a `.s` file, like so:

```
$ ./floralc main.floral asm_out.s
$
```

This will compile your code into an assembly file called `asm_out.s`. You then run the following commands on macOS, but the flow should be similar on other architectures. Simply compile and link the assembly output into an executable and then run it:

```
$ nasm -f macho64 asm_out.s -o asm_out.o
$ ld -o asm_out asm_out.o -lSystem
$ ./asm_out
Hello World!
$
```

