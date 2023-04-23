# Copyright (c) 2012, Ben Noordhuis <info@bnoordhuis.nl>
#
# Permission to use, copy, modify, and/or distribute this software for any
# purpose with or without fee is hereby granted, provided that the above
# copyright notice and this permission notice appear in all copies.
#
# THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
# WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
# MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
# ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
# WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
# ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
# OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

CPP	= cpp -P

CFLAGS	= -Wall -Wextra -std=c99
ASFLAGS	=
LDFLAGS	= -s -static

ifeq ($(shell uname -m),x86_64)
CFLAGS	+= -m32
ASFLAGS += -32
LDFLAGS	+=
endif

CFLAGS	+= -ffreestanding -nostartfiles -nostdlib -nodefaultlibs
CFLAGS	+= -Iklibc

CFLAGS	+= $(EXTRA_CFLAGS)
ASFLAGS += $(EXTRA_ASFLAGS)
LDFLAGS	+= $(EXTRA_LDFLAGS)

KLIBC_OBJS	= \
	klibc/atoi.o \
	klibc/atol.o \
	klibc/strtol.o \
	klibc/snprintf.o \
	klibc/vsnprintf.o \
	klibc/memcpy.o \
	klibc/memset.o \
	klibc/bzero.o \

.s.o:
	$(AS) $(ASFLAGS) -o $@ $<

.PHONY:	all
all:	multiboot.img

.PHONY:	clean
clean:
	rm -f *.a *.o *.img *.pp.{ld,s} klibc/*.o

.PHONY: qemu
qemu: multiboot.img
	qemu-system-i386 -m 16 -no-reboot -kernel $< -display curses -nographic -chardev stdio,mux=on,id=char0 -mon chardev=char0,mode=readline -serial chardev:char0

multiboot.img:	multiboot.pp.ld multiboot.o kern.o idt.o pic.o serial.o klibc.a
	$(LD) $(LDFLAGS) -o $@ -T $^

klibc.a:	$(KLIBC_OBJS)
	$(AR) cr $@ $^

multiboot.pp.ld:	multiboot.ld
	$(CPP) -o $@ $<

multiboot.o:	multiboot.pp.s
	$(AS) $(ASFLAGS) -o $@ $<

multiboot.pp.s:	multiboot.s
	$(CPP) -o $@ $<

kern.o:	kern.c kern.h
idt.o:	idt.c kern.h
pic.o:	pic.c kern.h
