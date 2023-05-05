CFLAGS = -Wall -Wextra -std=c11 -Os -mgeneral-regs-only -mno-red-zone -ffreestanding -nostdinc

LDFLAGS = -static -nodefaultlibs -nolibc -nostartfiles -nostdlib

.PHONY: all
all: multiboot32.elf

.PHONY: clean
clean:
	$(RM) kernel64.elf multiboot32.elf *.o

.PHONY: qemu
qemu: multiboot32.elf
	qemu-system-x86_64 -D qemu.log -d cpu_reset,int -m 16 -no-reboot -kernel $< -display curses -nographic -chardev stdio,mux=on,id=char0 -mon chardev=char0,mode=readline -serial chardev:char0

# multiboot32.elf incbins kernel64.elf at address 0x1FF000,
# putting the entry point at address 0x200000.

multiboot32.elf: multiboot32.c kernel64.elf
	$(CC) -m32 $(CFLAGS) -o $@ $< -Ttext=0x1FE000 $(LDFLAGS)

kernel64.elf: kernel64.c
	$(CC) -m64 $(CFLAGS) -o $@ $^ -Ttext=0x200000 $(LDFLAGS)
