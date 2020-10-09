#  Start
# WARNING: SPECIFICATION CHANGED

This is a guide which describes how one can compile and execute Floral. The requirements to do so are few: you will need a C++ compiler, a nasm compiler, and a linker.

First, write your floral code.

```
#include <io.fh>

func main(): Int {
    print("Hello, world!\n");
    return 0;
}
```

Then, run the compiler with your `.floral` file alongside a `.s` file, like so:

```
$ floralc main.floral -o asm_out
```

This will compile your code into an executable file called `asm_out`. Then, you run it.

```
$ ./asm_out
Hello World!
```

To link
nasm -f macho64 asm_out.nasm -o asm_out.o && ld -o asm_out asm_out.o -lSystem
nasm -f macho64 asm_out.nasm -o asm_out.o && ld -o asm_out asm_out.o ~/Programming/floral-src/stdlib/FILE.o -lSystem
nasm -f macho64 asm_out.nasm -o asm_out.o && ld -o asm_out asm_out.o ~/Programming/floral-src/stdlib/dynamic.o ~/Programming/floral-src/stdlib/sys.o -lSystem
nasm -f macho64 asm_out.nasm -o asm_out.o && ld -o asm_out asm_out.o ~/Programming/floral-src/stdlib/obj/std.o ~/Programming/floral-src/stdlib/obj/cbridge.o -lSystem
nasm -f macho64 asm_out.nasm -o asm_out.o && ld -o asm_out asm_out.o ~/Programming/floral-src/stdlib/int-print.o -lSystem
nasm -f macho64 asm_out.nasm -o asm_out.o && nasm -f macho64 asm_out2.nasm -o asm_out2.o && ld -o asm_out asm_out.o asm_out2.o ~/Programming/floral-src/stdlib/obj/std.o -lSystem
