section .text
extern _floralid_std#print_unbuffered_u ; @ std#print_unbuffered(&(const Char)): const Void

global _floralid_main
_floralid_main:
  push rbp ; store old frame
  mov rbp, rsp ; push new frame
  sub rsp, 16 ; allocate space on the stack for local variables
  mov qword [rbp-8], 0 ; @ var i: Int64
_floralid_main_#while_loop_0:
  cmp qword [rbp-8], 8388608
  jge _floralid_main_#while_skip_0
  lea rdi, [rel _floralid_#str_literal_0] ; string literal && argument 0
  call _floralid_std#print_unbuffered_u ; std#print_unbuffered(const Char[7])
  add qword [rbp-8], 1 ; add then assign
  jmp _floralid_main_#while_loop_0
_floralid_main_#while_skip_0:
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
  _floralid_#str_literal_0: db `ABC123\0`