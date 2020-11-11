# Name Mangling

Floral allows creation of functions with the same name as long as their type signature differs. The type signature is a combination of the function name and its parameter types (the return type has no effect).

Name mangling in floral is very simple: prepend `_floralid_` and append the corresponding type id string for each parameter. 

As an example, `func print(&const Char);` will generate:

```x86asm
extern _floralid_print_chptr
```

#### Mangling with C Interpolability

`floral_cdef.h` provides various macros to simulate Floral name mangling and thus allows creation of Floral functions in C.

To create a function called `foo` which no parameters, you do:

```c
#include "floral_cdef.h"

#define foo FloralIDn(foo)

FloralVoid foo() {

}
```

To create a function named `bar` which takes a c-string and an integer and then returns a boolean, you do:

```
#include "floral_cdef.h"

#define bar FloralID(bar, FloralTypeIDSuffixBuilder(FloralTypeIDPointerBuilder(FloralTypeIDChar), FloralTypeIDInt)

FloralBool bar(const char* str, FloralInt i) {
    return FloralTrue;
}
```