section .text
extern _floralid_print_u

extern _floralid_read_i_u

extern _floralid_wputchar_wch

extern _floralid_wprint_u

global _floralid_hello
_floralid_hello:
  push rbp ; store old frame
  mov rbp, rsp ; push new frame
  lea rax, [rel _floralid_#str_literal_0] ; string literal
  mov rdi, rax ; argument 0
  call _floralid_print_u ; print(const &Char)
  mov rsp, rbp ; pop this frame
  pop rbp ; restore old frame
  ret ; return from function

section .rodata
  _floralid_#str_literal_0: db `Hello\n\0`