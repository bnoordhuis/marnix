OUTPUT_FORMAT (elf32-i386)
OUTPUT_ARCH (i386)
ENTRY (entry)

SECTIONS
{
  .multiboot 0x80 :
  {
    *(.multiboot)
  }

  .text 0x10000 :
  {
    *(.text)
  }

  .rodata ALIGN (4096) :
  {
    *(.rodata*)
  }

  .data ALIGN (4096) :
  {
    *(.data*)
  }

  .bss ALIGN (4096) :
  {
    __bss_start = .;
    *(COMMON)
    *(.bss*)
    __bss_end = .;
    __kern_stack = . + 32768;
  }
}
