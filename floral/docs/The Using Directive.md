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
