section .text
global _floralid_main
_floralid_main:
  push rbp ; store old frame
  mov rbp, rsp ; push new frame
  sub rsp, 16 ; allocate space on the stack for local variables
  mov qword [rbp-8], 41 ; var number
  lea rax, [rbp-8] ; address of
  mov qword [rax], 42 ; assignment
  mov rax, [rbp-8] ; store number in rax
  mov rsp, rbp ; pop this frame
  pop rbp ; restore old frame
  ret ; return from function

extern _init_floral
global _main ; _main is the entry point in macOS nasm
_main:
  sub rsp, 8 ; @ so stack is aligned upon calls
  call _init_floral
  call _floralid_main ; @ call the floral main function
  add rsp, 8 ; @ restore stack pointer
  mov rdi, rax ; @ exit code
  mov eax, 0x2000001 ; @ exit syscall
  syscall