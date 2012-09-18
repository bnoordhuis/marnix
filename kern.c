__attribute__((aligned(8)))
unsigned char gdt[][8] = {
  // null descriptor
  {
  },

  // 4 GB execute/read code segment, nonconforming
  {
    [0] = 255,            // limit
    [1] = 255,            // limit
    [2] = 0,              // base
    [3] = 0,              // base
    [4] = 0,              // base
    [5] = 10 | 16 | 128,  // type | non-system | present
    [6] = 15 | 64 | 128,  // limit | 32 bits | granularity
    [7] = 0,              // base
  },

  // 4 GB read/write data segment
  {
    [0] = 255,            // limit
    [1] = 255,            // limit
    [2] = 0,              // base
    [3] = 0,              // base
    [4] = 0,              // base
    [5] = 2 | 16 | 128,   // type | non-system | present
    [6] = 15 | 64 | 128,  // limit | 32 bits | granularity
    [7] = 0,              // base
  },
};

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

static void load_gdt(void)
{
  const unsigned long base = (unsigned long) &gdt;
  const unsigned long size = sizeof(gdt) - 1;

  gdt[0][0] = size >> 0;
  gdt[0][1] = size >> 8;
  gdt[0][2] = base >> 0;
  gdt[0][3] = base >> 8;
  gdt[0][4] = base >> 16;
  gdt[0][5] = base >> 24;
  gdt[0][6] = 0;
  gdt[0][7] = 0;

  __asm__ __volatile__ (
    "lgdt gdt;"
    "jmpl $8, $.reload;"
    ".reload:"
  );
}

__attribute__((noreturn))
void kern_init(void)
{
  load_gdt();
  puts("Halting.");
  halt();
}
