__asm__ (".global _start;_start:jmp go"); // must be first

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

_Noreturn void
go(void)
{
	__asm__ __volatile__ ("outb %%al,%%dx;" :: "a" ('o'), "d" (0x3F8));
	__asm__ __volatile__ ("outb %%al,%%dx;" :: "a" ('k'), "d" (0x3F8));
	__asm__ __volatile__ ("outb %%al,%%dx;" :: "a" ('\n'), "d" (0x3F8));

	for (;;)
		__asm __volatile__ ("hlt");
}
