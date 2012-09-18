__asm__ (
  ".globl mb_init;"
  "mb_init:"
  "mov $__kern_stack, %esp;"
  "push $0;"
  "popf;"
  "push %ebx;"
  "push %eax;"
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
