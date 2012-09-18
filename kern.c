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

#define INTERRUPT_HANDLER(name)                                               \
  __asm__ (                                                                   \
    ".global __" #name ";"                                                    \
    "__" #name ":"                                                            \
    "pusha;"                                                                  \
    "call " #name ";"                                                         \
    "popa;"                                                                   \
  );                                                                          \
  void __##name(void);                                                        \
  void name(void)

#define SET_INTERRUPT_HANDLER(d, h)                                           \
  {                                                                           \
    (d)->r0 = (unsigned long) (__ ##h) >> 0;                                  \
    (d)->r3 = (unsigned long) (__ ##h) >> 16;                                 \
  }

#define SYSTEM_INTERRUPT(num)                                                 \
  INTERRUPT_HANDLER(sys_intr_##num)                                           \
  {                                                                           \
    puts("Unexpected interrupt " #num ".");                                   \
  }
  SYSTEM_INTERRUPT(0x00)
  SYSTEM_INTERRUPT(0x01)
  SYSTEM_INTERRUPT(0x02)
  SYSTEM_INTERRUPT(0x03)
  SYSTEM_INTERRUPT(0x04)
  SYSTEM_INTERRUPT(0x05)
  SYSTEM_INTERRUPT(0x06)
  SYSTEM_INTERRUPT(0x07)
  SYSTEM_INTERRUPT(0x08)
  SYSTEM_INTERRUPT(0x09)
  SYSTEM_INTERRUPT(0x0A)
  SYSTEM_INTERRUPT(0x0B)
  SYSTEM_INTERRUPT(0x0C)
  SYSTEM_INTERRUPT(0x0D)
  SYSTEM_INTERRUPT(0x0E)
  SYSTEM_INTERRUPT(0x0F)
  SYSTEM_INTERRUPT(0x10)
  SYSTEM_INTERRUPT(0x11)
  SYSTEM_INTERRUPT(0x12)
  SYSTEM_INTERRUPT(0x13)
  SYSTEM_INTERRUPT(0x14)
  SYSTEM_INTERRUPT(0x15)
  SYSTEM_INTERRUPT(0x16)
  SYSTEM_INTERRUPT(0x17)
  SYSTEM_INTERRUPT(0x18)
  SYSTEM_INTERRUPT(0x19)
  SYSTEM_INTERRUPT(0x1A)
  SYSTEM_INTERRUPT(0x1B)
  SYSTEM_INTERRUPT(0x1C)
  SYSTEM_INTERRUPT(0x1D)
  SYSTEM_INTERRUPT(0x1E)
  SYSTEM_INTERRUPT(0x1F)
#undef SYSTEM_INTERRUPT

INTERRUPT_HANDLER(ignore_spurious_interrupt)
{
  puts(__func__);
}

INTERRUPT_HANDLER(general_protection_fault)
{
  puts(__func__);
  halt();
}

INTERRUPT_HANDLER(syscall_entry)
{
  puts(__func__);
}

static void load_idt(void)
{
  struct descriptor *d;
  unsigned int i;

  for (i = 0; i < 32; i++) {
    d = idt + i;
    d->r1 = 8;       // kernel CS selector
    d->r2 = 0x8E00;  // present=1 | dpl=0 | type=intr_gate
    SET_INTERRUPT_HANDLER(d, ignore_spurious_interrupt);
  }

#define SYSTEM_INTERRUPT(num)                                                 \
  SET_INTERRUPT_HANDLER(idt + num, sys_intr_ ##num);
  SYSTEM_INTERRUPT(0x00)
  SYSTEM_INTERRUPT(0x01)
  SYSTEM_INTERRUPT(0x02)
  SYSTEM_INTERRUPT(0x03)
  SYSTEM_INTERRUPT(0x04)
  SYSTEM_INTERRUPT(0x05)
  SYSTEM_INTERRUPT(0x06)
  SYSTEM_INTERRUPT(0x07)
  SYSTEM_INTERRUPT(0x08)
  SYSTEM_INTERRUPT(0x09)
  SYSTEM_INTERRUPT(0x0A)
  SYSTEM_INTERRUPT(0x0B)
  SYSTEM_INTERRUPT(0x0C)
  SYSTEM_INTERRUPT(0x0D)
  SYSTEM_INTERRUPT(0x0E)
  SYSTEM_INTERRUPT(0x0F)
  SYSTEM_INTERRUPT(0x10)
  SYSTEM_INTERRUPT(0x11)
  SYSTEM_INTERRUPT(0x12)
  SYSTEM_INTERRUPT(0x13)
  SYSTEM_INTERRUPT(0x14)
  SYSTEM_INTERRUPT(0x15)
  SYSTEM_INTERRUPT(0x16)
  SYSTEM_INTERRUPT(0x17)
  SYSTEM_INTERRUPT(0x18)
  SYSTEM_INTERRUPT(0x19)
  SYSTEM_INTERRUPT(0x1A)
  SYSTEM_INTERRUPT(0x1B)
  SYSTEM_INTERRUPT(0x1C)
  SYSTEM_INTERRUPT(0x1D)
  SYSTEM_INTERRUPT(0x1E)
  SYSTEM_INTERRUPT(0x1F)
#undef SYSTEM_INTERRUPT

  for (i = 32; i < ARRAY_SIZE(idt); i++) {
    d = idt + i;
    d->r1 = 8;       // kernel CS selector
    d->r2 = 0xBF00;  // present=1 | dpl=3 | type=trap_gate
    SET_INTERRUPT_HANDLER(d, ignore_spurious_interrupt);
  }

  SET_INTERRUPT_HANDLER(idt + 0x0D, general_protection_fault);
  SET_INTERRUPT_HANDLER(idt + 0x80, syscall_entry);

  store_addr(idt_addr, &idt, sizeof(idt) - 1);
  __asm__ __volatile__ ("lidt idt_addr");
}

__attribute__((noreturn))
void kern_init(void)
{
  load_gdt();
  load_idt();
  puts("Halting.");
  halt();
}
