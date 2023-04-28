typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

// order matters, see int80enter() and usermode()
struct regs
{
	u32 ax;
	u32 bx;
	u32 cx;
	u32 dx;
	u32 si;
	u32 di;
	u32 bp;
	u32 ip;
	u32 cs;
	u32 flags;
	u32 sp;
	u32 ss;
};

struct dt
{
	u16 len;
	void *ptr;
}
__attribute__((packed));

inline static u8
inb(u16 port)
{
	u8 val;
	__asm__ __volatile__ ("inb %1,%0" : "=a" (val) : "Nd" (port));
	return val;
}

inline static void
outb(u16 port, u8 val)
{
	__asm__ __volatile__ ("outb %0,%1" : /* no output */ : "a" (val), "Nd" (port));
}

inline static void
iowait(void)
{
	outb(0x80, 0);
}

inline static u32
get_cr0(void)
{
	u32 val;
	__asm__ __volatile__ ("mov %%cr0,%0" : "=r" (val));
	return val;
}

inline static void
set_cr0(u32 val)
{
	__asm__ __volatile__ ("mov %0,%%cr0" : /* no output */ : "r" (val));
}

inline static void
set_cr3(u32 val)
{
	__asm__ __volatile__ ("mov %0,%%cr3" : /* no output */ : "r" (val));
}

inline static u32
get_cr4(void)
{
	u32 val;
	__asm__ __volatile__ ("mov %%cr4,%0" : "=r" (val));
	return val;
}

inline static void
set_cr4(u32 val)
{
	__asm__ __volatile__ ("mov %0,%%cr4" : /* no output */ : "r" (val));
}

void
initserial(void);

void
initpic(void);

void
initgdt(void);

void
initidt(void);

void
initpaging(void);

void
print(const char *);

_Noreturn void
resched(void);

_Noreturn void
usermode(struct regs);

_Noreturn __attribute__((regparm(1))) void
kernelmode(__attribute__((noreturn)) void (*)(void));

static struct regs procs[2];
static int ff;

__asm__ (".long 0x1BADB002,0,-(0x1BADB002+0)"); // multiboot header

_Noreturn void
start(void)
{
	initserial();
	initpaging();
	initgdt();
	initidt();
	initpic();

	print("all systems go\n");

	{
		static u8 program[] = {
			0xb8,0xbe,0xba,0xad,0xde, // mov eax,0xdeadbabe
			0xb9,0x41,0x00,0x00,0x00, // mov ecx,'A'
			0xcd,0x80,                // int 0x80
			0xeb,0xfc,                // jmp $-4
			0xf4,                     // hlt
		};
		u32 ip = 8u << 20;
		u8 *s = program;
		u8 *d = (u8 *) ip;
		while (s != program + sizeof(program))
			*d++ = *s++;
	}

	{
		static u8 program[] = {
			0xb8,0xbe,0xba,0xad,0xde, // mov eax,0xdeadbabe
			0xb9,0x42,0x00,0x00,0x00, // mov ecx,'B'
			0xcd,0x80,                // int 0x80
			0xeb,0xfc,                // jmp $-4
			0xf4,                     // hlt
		};
		u32 ip = 12u << 20;
		u8 *s = program;
		u8 *d = (u8 *) ip;
		while (s != program + sizeof(program))
			*d++ = *s++;
	}

	procs[0] = (struct regs)
	{
		.sp = (12u << 20) - 4,
		.ip = 8u << 20,
		.cs = 0x18|3,
		.ss = 0x20|3,
		.flags = 0x200, // IF
	};

	procs[1] = (struct regs)
	{
		.sp = (16u << 20) - 4,
		.ip = 12u << 20,
		.cs = 0x18|3,
		.ss = 0x20|3,
		.flags = 0x200, // IF
	};

	usermode(procs[0]);
}

#define PORT 0x3F8 // COM1

void
initserial(void)
{
	outb(PORT + 1, 0x00);
	outb(PORT + 3, 0x80);
	outb(PORT + 0, 0x03);
	outb(PORT + 1, 0x00);
	outb(PORT + 3, 0x03);
	outb(PORT + 2, 0xC7);
	outb(PORT + 4, 0x0B);
	outb(PORT + 4, 0x1E);
	outb(PORT + 0, 0xAE);
	outb(PORT + 4, 0x0F);
	print("\n");
	print("serial ok\n");
}

void
putc(int c)
{
	for (;;)
		if (32 & inb(PORT + 5))
			break;
	outb(PORT + 0, c);
}

void
print(const char *s)
{
	while (*s)
		putc(*s++);
}

void
printnum(u32 x)
{
	static const char hex[] = "0123456789abcdef";
	u32 shift;
	u8 b;

	shift = 32;
	do
	{
		shift -= 8;
		b = 255 & (x >> shift);
		putc(hex[b >> 4]);
		putc(hex[b & 15]);
	}
	while (shift != 0);
}

void
initpic(void)
{
	u8 mask[2];

	mask[0] = inb(0x21);
	mask[1] = inb(0xa1);

	outb(0x20, 0x11);
	outb(0xA0, 0x11);
	iowait();

	outb(0x21, 0x20); // map to interrupt range 0x20-0x27
	outb(0xA1, 0x28); // map to interrupt range 0x28-0x2F
	iowait();

	outb(0x21, 4);    // have slave at IRQ2
	outb(0xA1, 2);    // ping master through IRQ2
	iowait();

	outb(0x21, 1);    // 8086 mode
	outb(0xA1, 1);    // ditto
	iowait();

	outb(0x21, mask[0]);
	outb(0xA1, mask[1]);
	iowait();

	print("pic ok\n");
}

__attribute__((naked)) static void
irq0(void)
{
	outb(0x20, 0x20); // ack IRQ
	__asm__ __volatile__ ("iret");
}

__attribute__((aligned(4096))) static u32 pd[1024] =
{
	[0x000] = 0x00000080u, // 4 MB zero page; ps=1
	[0x001] = 0x00400081u, // 4-8 MB identity-mapped; present, ro, supervisor, ps=1
	[0x002] = 0x00800087u, // 8-12 MB identity-mapped; present, rw, user, ps=1
	[0x003] = 0x00C00087u, // 12-16 MB identity-mapped; present, rw, user, ps=1
	[0x3FF] = 0xC0000083u, // first 0-4 MB mapped at 0xC000_0000; present, rw, supervisor, ps=1
};

void
initpaging(void)
{
	set_cr3((u32) &pd);
	set_cr4(0x00000010u | get_cr4()); // turn on 4 MB pages (PSE)
	set_cr0(0x80000001u | get_cr0()); // turn on paging and protected mode
	print("paging ok\n");
}

__attribute__((aligned(4096))) static u8 stack[8192];

static u32 tss[26] =
{
	[1] = (u32) &stack + sizeof(stack), // sp = kernel stack
	[2] = 16,                           // ss = kernel ds
};

_Static_assert(104 == sizeof(tss));

static u64 gdt[] = {
	0x0000000000000000ull, // null descriptor
	0x00CF9A000000FFFFull, // kernel cs
	0x00CF92000000FFFFull, // kernel ds
	0x00CFFA000000FFFFull, // user cs
	0x00CFF2000000FFFFull, // user ds
	0x0000E90000000068ull, // tss; see initgdt()
};

void
initgdt(void)
{
	gdt[5] |= ((u64) (u32) &tss) << 16;
	__asm__ __volatile__ (
		//"lgdt %0;"
		"lgdt %a0;"
		"jmpl $8,$1f;" // kernel cs
		"1:"
		"mov $16,%%ax;" // kernel ds
		"mov %%ax,%%ss;"
		"mov %%ax,%%ds;"
		"mov %%ax,%%es;"
		"mov %%ax,%%fs;"
		"mov %%ax,%%gs;"
		"mov $43,%%ax;" // tss + rpl=3
		"ltr %%ax;"
		: // no output
		: "g" (&(struct dt){sizeof(gdt)-1, &gdt})
		: "ax"
	);
	print("gdt ok\n");
}

__attribute__((naked)) static void
trap(void)
{
	__asm__ __volatile__ ("iret");
}

__attribute__((naked, no_caller_saved_registers)) void
invalidopcode(void)
{
	__asm__ __volatile__ ("cli; hlt");
}

_Noreturn void
int80enter(void);

__attribute__((no_caller_saved_registers)) void
int80(struct regs r)
{
	static u32 count;
	putc(r.cx);
	procs[ff] = r;
	switch (++count % 4)
	{
		case 0: case 3:
			resched();
			__builtin_unreachable();
		case 1:
			break;
		case 2:
			kernelmode(resched);
			__builtin_unreachable();
	}
}

inline static _Bool
iskernelmode(void)
{
	u16 cs;
	__asm__ __volatile__ ("mov %%cs,%0" : "=r" (cs));
	return cs == 0x08;
}

_Noreturn void
resched(void)
{
	if (iskernelmode())
	{
		putc('K');
		__asm__ __volatile__ ("sti; hlt; cli"); // wait for timer tick
	}

	ff = !ff;
	usermode(procs[ff]);
}

static u64 idt[256];

static void
set_idt(u8 num, u32 isr)
{
	idt[num] = 0;
	idt[num] |= (u64) isr & 0xFFFFu;
	idt[num] |= (u64) isr >> 16 << 48;
	idt[num] |= 8ull << 16; // kernel cs
	idt[num] |= 0xEEull << 40; // flags
}

void
initidt(void)
{
	for (u32 i = 0; i < 256; i++)
		set_idt(i, (u32) &trap);

	set_idt(0x06, (u32) &invalidopcode); // PIC
	set_idt(0x20, (u32) &irq0); // PIC
	set_idt(0x80, (u32) &int80enter);

	__asm__ __volatile__ (
		"lidt %a0"
		: // no output
		: "g" (&(struct dt){sizeof(idt)-1, &idt})
	);

	print("idt ok\n");
}

__asm__ (
	".global _start;"
	"_start:"
	"mov $stack,%eax;"
	"add $8192,%eax;" // sizeof(stack)
	"mov %eax,%esp;"
	"jmp start;"
);

__asm__(
	"int80enter:"
	"push %ebp;"
	"push %edi;"
	"push %esi;"
	"push %edx;"
	"push %ecx;"
	"push %ebx;"
	"push %eax;"
	"call int80;"
	"int80exit:"
	"pop %eax;"
	"pop %ebx;"
	"pop %ecx;"
	"pop %edx;"
	"pop %esi;"
	"pop %edi;"
	"pop %ebp;"
	"iret;"
);

// _Noreturn void usermode(struct regs);
__asm__(
	"usermode:"
	"add $4,%esp;"
	"jmp int80exit;"
);

// _Noreturn __attribute__((regparm(1))) void kernelmode(__attribute__((noreturn)) void (*)(void));
__asm__(
	"kernelmode:"
	"push $0;"     // no flags
	"push $8;"     // kernel cs
	"push %eax;"   // ip
	"mov $16,%ax;" // kernel ds
	"mov %ax,%ds;"
	"iret;"
);
