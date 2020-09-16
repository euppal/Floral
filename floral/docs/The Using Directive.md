# The 'Using' Directive

Floral provides a way to access certain APIs with the `using` directive.

Syntax (BNF):
```
<using-directive> ::= 'using' <id> ';'
```

These APIs include:

| Directive | Functionality |
| --------- | ------------- |
| _using stdlib;_ | This directive provides access to the standard library if explicitly disabled via a compiler flag. |
| _using C;_ | This directive provides access to various C functions. Some examples of such functions include: `malloc`, `free`, `strcmp`, `strcpy`, `strlen`, `clock`.
| _using syscalls;_ | This directive provides access to functions which invoke the operating system. Use of this API is not recommended. Some functions provided are `syscall_exit`, `syscall_write` and `syscall_mmap`. |
| _using math;_ | This directive provides access to various math operations implemented for double-precision floating-point numbers such as `exp`, `log10`, `pythag`, `sin` and `acos`. |