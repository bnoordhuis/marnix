__asm__ (
  ".globl entry;"
  "entry:"
  "jmp multiboot_init;"
  ".align 8;"
  ".long 0x1BADB002;"
  ".long 3;"    // PAGE_ALIGN | MEMORY_INFO
  ".long -(0x1BADB002 + 3);"
  ".long 0;"
  ".long 0;"
  ".long 0;"
  ".long 0;"
  ".long 0;"
  ".long 1;"    // EGA mode
  ".long 80;"   // # of columns
  ".long 25;"   // # of rows
  ".long 0;"
  ".align 4;"
  "multiboot_init:"
  "mov __kern_stack, %esp;"
  "push $0;"
  "popf;"
  "jmp kern_init;"
);

__attribute__((unused))
__attribute__((always_inline))
static unsigned char inb(unsigned short port)
{
  unsigned char val;

  __asm__ __volatile__ (
    "inb %1, %0"
    : "=a" (val)
    : "Nd" (port)
  );

  return val;
}

__attribute__((unused))
__attribute__((always_inline))
static void outb(unsigned short port, unsigned char val)
{
  __asm__ __volatile__ (
    "outb %0, %1"
    : // no output
    : "a" (val), "Nd" (port)
  );
}

static void putc(int c)
{
  outb(0xE9, c);
}

static void puts(const char *s)
{
  while (*s) putc(*s++);
  putc('\n');
}

__attribute__((unused))
__attribute__((noreturn))
static void halt(void)
{
  for (;;)
    __asm__ __volatile__ ("cli; hlt");
}

__attribute__((noreturn))
void kern_init(void)
{
  puts("Hello, world!");
  halt();
}
