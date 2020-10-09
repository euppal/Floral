# The Floral Compiler (floralc)
# WARNING: SPECIFICATION CHANGED

The Floral compiler is a program which converts floral programs into machine code. This article documents its usage.

To run the compiler, simply type `./floralc -o outfile infile [infiles] [flags]`. This command receives floral and/or C programs as input and the name of the executable file as output.

#### Compiler Flags

There are a plurality of compiler flags to customize the behavior of the Floral compiler.

##### `-O`
Passing this flag to the Floral compiler tells it to make optimizations during compilation.

##### `--use-C`
Passing this flag causes the compiler to link with a C bridge.

##### `--no-stdlib`
Passing this flag disables default linking with the standard library 

##### `--verbose`
Passing this flag dumps the AST for debugging.

##### `--type-trace`
Passing this flag dumps the static analyzer type trace for debugging.

##### `--cat-src`
Passing this flag dumps the preprocessed source material for debugging.
