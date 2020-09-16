# The Floral Compiler (floralc)

The Floral compiler is a program which converts floral programs into machine code. This article documents its usage.

To run the compiler, simply type `./floralc infile outfile`. This command receives a floral program as input and the name of the assembly file as output.

#### Compiler Flags

There are a plurality of compiler flags to customize the behavior of the Floral compiler.

##### `-O`
Passing `-O` to the Floral compiler tells it to make optimizations during compilation. There are no optimization levels (`-O1`, `-O2`, etc) or an optimization targeting certain aspects (`-Osize`, `-Ofast`), simply `-O`.

##### `--use-C`

##### `--no-stdlib-header`