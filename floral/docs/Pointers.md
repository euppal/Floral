# Floral Pointers

This document assumes general familiarity with pointers. Floral pointers and their syntax are very similar to those in C.


Floral | C | Description
------ | - | -----------
`&Type` | `Type*` | A pointer type
`&value` | `&value` | Address of value (reference)
`*ptr` | `*ptr` | Value at address (dereference)

##### Pointer Arithmetic

Pointers are implemented as `UInt`s and thus many standard arithmetic operations can be performed on them. You can even add and subtract pointers.

```
func main(): Int {
    var n: CArray<Int>(2); // create a C-style static array (long n[2];)
    n[0] = 1; n[1] = 4; // initialize the array

    var ptr = &n; // get address of static array
    ptr++; // increment the pointer
    print(*n, '\n'); // prints '4'
}
```

They can even be bitcasted to `UInt`s.

```
let uint = bit_cast<UInt>(ptr);
```

In Floral, 0 is never a valid address. Thus, you can use 0 to indicate pointer errors or absence of value. 0 is also represented by the macro `null`.

```
#define null 0
```