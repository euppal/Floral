#  Start

This is a guide which describes how one can compile and execute Floral. 

First, write your floral code.

```
#include <io.fh>

func main(): Int {
    print("Hello, world!\n");
    return 0;
}
```

Then, run the compiler with your `.floral` file alongside a target executable, like so:

```
$ floralc -o out main.floral
```

This will compile your code into an executable file called `out`. Then, you run it.

```
$ ./out
Hello World!
```
