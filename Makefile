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
	rm -f *.a *.o *.img klibc/*.o

multiboot.img:	multiboot.ld kern.o klibc.a
	$(LD) $(LDFLAGS) -o $@ -T $^

klibc.a:	$(KLIBC_OBJS)
	$(AR) cr $@ $^
