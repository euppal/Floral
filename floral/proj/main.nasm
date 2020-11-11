section .text
extern _floralid_std#buffered_print_u ; @ std#buffered_print(&(const Char)): const Void
extern _floralid_std#flush ; @ std#flus): const Void

global _floralid_main
_floralid_main:
  push rbp ; store old frame
  mov rbp, rsp ; push new frame
  lea rdi, [rel _floralid_#str_literal_0] ; string literal && argument 0
  call _floralid_std#buffered_print_u ; std#buffered_print(const Char[6])
  call _floralid_std#flush ; std#flush(Void)
  xor eax, eax ; result to be returned
  mov rsp, rbp ; pop this frame
  pop rbp ; restore old frame
  ret ; return from function

extern _init_floral ; initialization procedure
global _main ; _main is the entry point in macOS nasm
_main:
  sub rsp, 8 ; @ so stack is aligned upon calls
  call _init_floral ; @ call initialization procedure
  call _floralid_main ; @ call the floral main function
  add rsp, 8 ; @ restore stack pointer
  mov rdi, rax ; @ exit code
  mov eax, 0x2000001 ; @ exit syscall
  syscall

section .rodata
  _floralid_#str_literal_0: db `Hey\n\0`