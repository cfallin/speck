# arch/i386/Makefile

srcroot = ../..
include ../../make.inc

OBJS = start.o init.o paging.o int.o int_handler.o context.o string.o panic.o vid.o lock.o elf.o

.PHONY: all
all: kernel

kernel: $(OBJS) ../kernel.o
	$(LD) -Tkernel.ld -o kernel $(OBJS) ../kernel.o
	cp kernel $(srcroot)/bin/i386/sys/kernel/kernel

%.o: %.asm
	nasm -f elf -i $(srcroot)/include/ -o $@ $^

.PHONY: clean
clean:
	rm -f *.o *~ kernel

.PHONY: setup
setup:
