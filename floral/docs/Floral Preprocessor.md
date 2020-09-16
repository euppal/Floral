# Floral Preprocessor

Floral contains a limited preprocessor for utility. Preprocessing directives begin with `#`.

The following is a list of every such directive:
- `#line`: Replaced by a decimal integer, the line number.
- `#column`: Replaced by a decimal integer, the column number.
- `#file`: Replaced by a string literal, the file path.
- `#include`: Includes the contents of the specified file in the current file.
- `#define`: Defines a macro without arguments.
- `#undef`: Undefines a macro.
- `#ifdef`: Removes the code enclosed by the `#ifdef` and `#endif` if the macro specified **is not** defined.
- `#ifndef`: Removes the code enclosed by the `#ifndef` and `#endif` if the macro specified **is** defined.
- `#endif`: A directive indicates the end of an entire `#if` directive.