# Floral

> Hello world in Floral.
```
func main(): Int {
    print("Hello, world!\n");
    return 0;
}
```

Floral is a strongly and statically typed compiled programming language inspired by C and Swift. The language is [open source](https://github.com/euppal/floral). It aims to draw on and expand on many existing features within the C family of languages while also providing slightly different syntax as to not lose familiarity but gain individuality. The wiki documents many aspects of the Floral language as well as how to get up and running with it, and as such is a very useful tool. You can find it [here]().

Floral compiles to x86_64 intel assembly code. The output location can be specified. There is capability for single file compiliation or project compiliation.

### Example Programs

The following are example Floral programs, each portraying a different aspect of the language.

> Fibbonaci
```
func fib(x: Int): Int {
    if (x > 1) {
        return fib(x-1) + fib(x-2);
    } else {
        return x;
    }
}

func main(): Int {
    print(fib(10), '\n');
    return 0;
}
```

> File IO
```
import Foundation;

func main(): Int {
    let url = FileURL("Path/To/File.txt")
    let data = "Hello, world!".cstr();
    try data.write(url) catch { return 1; };
    return 0;
}
```

> Generics
```
func dotProduct<T: Numeric>(x: T, y: T): T {
    return x * x + y * y;
}

func main(): Int {
    print(dotProduct(3, 4), '\n');
    return 0;
}
```
