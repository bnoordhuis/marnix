#define EFER 0xC0000080u

__asm__ (".long 0x1BADB002,0,-(0x1BADB002+0)"); // multiboot header
__asm__ (".align 4096; kernel64:.incbin \"kernel64.elf\"");

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

struct elfehdr64
{
	u8 ident[16];
	u16 type;
	u16 machine;
	u32 version;
	u64 entry;
	u64 phoff;
	u64 shoff;
	u32 flags;
	u16 ehsize;
	u16 phentsize;
	u16 phnum;
	u16 shentsize;
	u16 shnum;
	u16 shstrndx;
};

_Static_assert(64 == sizeof(struct elfehdr64));

extern struct elfehdr64 kernel64;

struct gdtptr
{
	u16 size;
	void *addr;
} __attribute__((packed));

_Static_assert(6 == sizeof(struct gdtptr));

// 64 bits code segment limit is for initial 32 bits -> 64 bits jump
static u64 gdt[] __attribute__((aligned(16))) =
{
	0x0000000000000000ull,
	0x00AF9A000000FFFFull, // 64 bits code segment
	0x00CF92000000FFFFull, // 32 bits data segment
};

static u64 p4[512] __attribute__((aligned(4096)));
static u64 p3[512] __attribute__((aligned(4096)));
static u8 stack[512] __attribute__((aligned(32)));

u32
getcr0(void)
{
	u32 val;
	__asm__ __volatile__ ("mov %%cr0,%0" : "=a" (val));
	return val;
}

void
setcr0(u32 val)
{
	__asm__ __volatile__ ("mov %0,%%cr0" : /* no output */ : "a" (val));
}

void
setcr3(void *val)
{
	__asm__ __volatile__ ("mov %0,%%cr3" : /* no output */ : "a" (val));
}

u32
getcr4(void)
{
	u32 val;
	__asm__ __volatile__ ("mov %%cr4,%0" : "=a" (val));
	return val;
}

void
setcr4(u32 val)
{
	__asm__ __volatile__ ("mov %0,%%cr4" : /* no output */ : "a" (val));
}

u32
rdmsr(u32 nr)
{
	u32 val;
	__asm__ __volatile__ ("rdmsr" : "=a" (val) : "c" (nr));
	return val;
}

void
wrmsr(u32 nr, u32 val)
{
	__asm__ __volatile__ ("wrmsr" : /* no output */ : "a" (val), "c" (nr));
}

void
lgdt(struct gdtptr p)
{
	__asm__ __volatile__ ("lgdt %0" : /* no output */ : "g" (p));
}

u8
inb(u16 port)
{
	u8 val;
	__asm__ __volatile__ ("inb %1,%0" : "=a" (val) : "Nd" (port));
	return val;
}

void
outb(u16 port, u8 val)
{
	__asm__ __volatile__ ("outb %0,%1" : /* no output */ : "a" (val), "Nd" (port));
}

_Noreturn void
hang(void)
{
	for (;;)
		__asm__ __volatile__ ("hlt");
}

void
putc(int c)
{
	for (;;)
		if (32 & inb(0x3FD))
			break;
	outb(0x3F8, c);
}

void
print(const char *s)
{
	while (*s)
		putc(*s++);
}

_Noreturn void
panic(const char *s)
{
	print("panic: ");
	print(s);
	print("\n");
	hang();
}

void
printnum(u64 x)
{
	static const char hex[] = "0123456789abcdef";
	u32 shift;
	u8 b;

	shift = 64;
	do
	{
		shift -= 8;
		b = 255 & (x >> shift);
		putc(hex[b >> 4]);
		putc(hex[b & 15]);
	}
	while (shift != 0);
}

_Noreturn void
_start(void)
{
	// move to a stack that is in the memory range we're going to map
	__asm__ __volatile__ (
		"mov %0,%%esp;"
		"jmp go;"
		: // no output
		: "g" (stack + sizeof(stack))
		: "memory"
	);
	__builtin_unreachable();
}

_Noreturn void
go(void)
{
	// configure serial port so we can print
	outb(0x3F9, 0x00);
	outb(0x3FB, 0x80);
	outb(0x3F8, 0x03);
	outb(0x3F9, 0x00);
	outb(0x3FB, 0x03);
	outb(0x3FA, 0xC7);
	outb(0x3FC, 0x0B);
	outb(0x3FC, 0x1E);
	outb(0x3F8, 0xAE);
	outb(0x3FC, 0x0F);

	print("\nserial ok\n");

	if ((u32) &kernel64 != 0x1FF000u)
		panic("bad kernel64 alignment");

	if (kernel64.entry != 0x200000u)
		panic("bad kernel64 entry point");

	// TODO map kernel64 ELF sections file to their proper virtual addresses

	setcr3(p4);
	p4[0] = (u64) (u32) &p3 | 3;
	p3[0] = 0u<<20 | 0x83;
	p3[1] = 2u<<20 | 0x83;
	p3[2] = 4u<<20 | 0x83;

	lgdt((struct gdtptr){sizeof(gdt)-1, &gdt});

	setcr4(0x0000020u | getcr4());    // turn on PAE
	wrmsr(EFER, 0x100 | rdmsr(EFER)); // turn on long mode
	setcr0(0x8000000u | getcr0());    // turn on paging

	__asm__ __volatile__ (
		"mov $16,%ax;"
		"mov %ax,%ds;"
		"mov %ax,%es;"
		"mov %ax,%fs;"
		"mov %ax,%gs;"
		"mov %ax,%ss;"
		"ljmp $8,$0x200000;"
	);

	panic("unreachable");
}
