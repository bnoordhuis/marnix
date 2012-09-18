.section .multiboot, "ax"
.globl entry

entry:
  jmp multiboot_init

.balign 8
  .long 0x1BADB002
  .long 3    # PAGE_ALIGN | MEMORY_INFO
  .long -(0x1BADB002 + 3)
  .long 0
  .long 0
  .long 0
  .long 0
  .long 0
  .long 1    # EGA mode
  .long 80   # no. of columns
  .long 25   # no. of rows
  .long 0
  .align 4

multiboot_init:
  mov $__multiboot_stack, %esp

  # reset flags to known-good state
  push $0
  popf

  call kern_init

halt:
  cli
  hlt
  jmp halt

.section data
.space 4096
__multiboot_stack:
