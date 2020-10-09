section .text
extern _floralid_hello

global _floralid_main
_floralid_main:
  push rbp ; store old frame
  mov rbp, rsp ; push new frame
  call _floralid_hello ; hello(Void)
  xor eax, eax ; result to be returned
  mov rsp, rbp ; pop this frame
  pop rbp ; restore old frame
  ret ; return from function

global _main ; _main is the entry point in macOS nasm
_main:
  sub rsp, 8 ; so stack is aligned upon call
  call _floralid_main ; call the floral main function
  add rsp, 8 ; restore stack pointer
  mov rdi, rax ; exit code
  mov eax, 0x2000001 ; exit syscall
  syscall