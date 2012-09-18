OUTPUT_FORMAT (elf32-i386)
OUTPUT_ARCH (i386)

SECTIONS
{
  . = SIZEOF_HEADERS;

  .multiboot ALIGN(16) :
  {
    LONG(0x1BADB002);               /* multiboot magic */
    LONG(0x10003);                  /* align | meminfo | address */
    LONG(-(0x1BADB002 + 0x10003));  /* checksum */

    /* address */
    LONG(ADDR(.multiboot));
    LONG(ADDR(.text));
    LONG(ADDR(.data) + SIZEOF(.data));
    LONG(ADDR(.bss) + SIZEOF(.bss));
    LONG(kern_init);
  }

  .text 0x100000 :
  {
    *(.text)
  }

  .rodata ALIGN (4096) :
  {
    *(.rodata*)
  }

  .data :
  {
    *(.data*)
  }

  .bss :
  {
    *(COMMON)
    *(.bss*)

    . = ALIGN(4096);
    __kern_stack = . + 32768;
  }
}
