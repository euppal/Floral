# Floral Headers

The Floral compiler compiles on a per-file basis. Hence, the use of external functions requires a workaround. Floral, like C, allows you to forward declare functions and structures. 

The syntax for forward-declaring a function is as follows:

```
func function(arg1: Type1, arg2: Type2): ReturnType;
```

The syntax for forward-declaring a structure is as follows:

```
predeclare struct Foo {
    Foo(arg1: Type1, arg2: Type2);
    ~Foo();
    var dataMember: Type3;
    func functionMember(arg1: Type1, arg2: Type2): ReturnType;
};
```

Note the addition of the `predeclare` keyword. This is so a `struct` can be forward declared in multiple files without being interpreted as an ordinary `struct`.

However, this solution gets quite repetitive if multiple files wish to use the same structures and functions. To avoid this problem, place all your forward declarations in a floral header (`.fh`) file and `#include` them into your code files.

`calc.floral`:
```
func dotprod(a: Double, b: Double): Double {
    return a * a + b * b
}
```

`calc.fh`:
```
func dotprod(a: Double, b: Double): Double;
```

`main.floral`:
```
#include "calc.fh"

func main(): Int {
    let hypotenuse = sqrt(dotprod(3, 4));
    print(hypotenuse, '\n');
    return 0;
}
```

To avoid an `#include` cycle where `a.fh` and `b.fh` both request the preprocessor to dump the contents of the other file in them, header guards may be used.

`a.fh`:
```
#ifndef a_fh
#define a_fh

#include "b.fh"

#endif // a_fh
```

`b.fh`:
```
#ifndef b_fh
#define b_fh

#include "a.fh"

#endif // b_fh
```

Thus, if `a.fh` is preprocessed first,
1. `a.fh` checks to see if the macro `a_fh` is defined. It isn't, so it gets defined.
2. `a.fh` includes `b.fh`.
3. `b.fh` checks to see if the macro `b_fh` is defined. It isn't, so it gets defined.
4. `b.fl` includes `a.fh`.
5. `b.fh` checks to see if the macro `a_fh` is defined. It is, so nothing gets included.