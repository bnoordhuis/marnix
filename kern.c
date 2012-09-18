#define ARRAY_SIZE(a)                                                         \
  (sizeof(a) / sizeof((a)[0]))

__attribute__((packed))
struct descriptor
{
  unsigned short r0;
  unsigned short r1;
  unsigned short r2;
  unsigned short r3;
};

__attribute__((aligned(8)))
struct descriptor gdt[] =
{
  // null descriptor
  {
    .r0 = 0,
    .r1 = 0,
    .r2 = 0,
    .r3 = 0,
  },

  // 4 GB execute/read code segment, nonconforming
  {
    .r0 = 0xFFFF, // limit
    .r1 = 0x0000, // base
    .r2 = 0x9A00, // type | non-system | present
    .r3 = 0x00CF, // limit | 32 bits | granularity
  },

  // 4 GB read/write data segment
  {
    .r0 = 0xFFFF, // limit
    .r1 = 0x0000, // base
    .r2 = 0x9200, // type | non-system | present
    .r3 = 0x00CF, // limit | 32 bits | granularity
  },
};

__attribute__((aligned(8)))
struct descriptor idt[256];

__attribute__((aligned(8)))
unsigned char idt_addr[6];

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

static void store_addr(unsigned char dst[6], void *addr, unsigned short size)
{
  dst[0] = size >> 0;
  dst[1] = size >> 8;
  dst[2] = (unsigned long) addr >> 0;
  dst[3] = (unsigned long) addr >> 8;
  dst[4] = (unsigned long) addr >> 16;
  dst[5] = (unsigned long) addr >> 24;
}

static void load_gdt(void)
{
  store_addr((unsigned char *) &gdt, &gdt, sizeof(gdt) - 1);

  __asm__ __volatile__ (
    "lgdt gdt;"
    "jmpl $8, $.reload;"
    ".reload:"
  );
}

#if 0
__asm__ (
  ".globl ignore_interrupt;"
  "ignore_interrupt:"
  "iret;"
);
void ignore_interrupt(void);
#else
__asm__ (
  ".globl ignore_interrupt;"
  "ignore_interrupt:"
  "pusha;"
  "call print_warning;"
  "popa;"
  "iret;"
);
void ignore_interrupt(void);
void print_warning(void)
{
  puts("received interrupt");
}
#endif

static void load_idt(void)
{
  struct descriptor *d;
  unsigned int i;

  for (i = 0; i < 32; i++) {
    d = idt + i;
    d->r0 = (unsigned long) ignore_interrupt >> 0;
    d->r1 = 8;       // kernel CS selector
    d->r2 = 0x8600;  // type=intr_gate | dpl=0 | present=1
    d->r3 = (unsigned long) ignore_interrupt >> 16;
  }

  for (i = 32; i < ARRAY_SIZE(idt); i++) {
    d = idt + i;
    d->r0 = (unsigned long) ignore_interrupt >> 0;
    d->r1 = 8;       // kernel CS selector
    d->r2 = 0xB700;  // type=trap_gate | dpl=3 | present=1
    d->r3 = (unsigned long) ignore_interrupt >> 16;
  }

  store_addr(idt_addr, &idt, sizeof(idt) - 1);
  __asm__ __volatile__ ("lidt idt_addr");
}

__attribute__((noreturn))
void kern_init(void)
{
  puts("load gdt");
  load_gdt();
  puts("load idt");
  load_idt();
  puts("Halting.");
  halt();
}
