CFLAGS = -Wall -Wextra -std=c11 -Os -mgeneral-regs-only -mno-red-zone -ffreestanding -nodefaultlibs -nostartfiles -nostdinc -nostdlib -m32

LDFLAGS = -static -Ttext=0x400000 -nostdinc -nodefaultlibs -nostartfiles -nostdlib -m32

.PHONY: all
all: kernel.elf

.PHONY: clean
clean:
	$(RM) kernel.elf *.o

.PHONY: qemu
qemu: kernel.elf
	qemu-system-i386 -D qemu.log -d cpu_reset,int -m 16 -no-reboot -kernel $< -display curses -nographic -chardev stdio,mux=on,id=char0 -mon chardev=char0,mode=readline -serial chardev:char0

kernel.elf: start.o
	$(CC) -o $@ $^ $(LDFLAGS)
