# bin/i386/Makefile

srcroot = ../..
include ../../make.inc

.PHONY: all
all: floppy.img tftpboot

floppy.img: floppybase.gz $(BINARIES) menu.lst
	(cat floppybase.gz | gunzip -cd > floppy.img)
	(export MTOOLSRC=mtoolsrc; mcopy -o -s sys s: ; mcopy -o menu.lst s:/boot/grub)

.PHONY: tftpboot
tftpboot: $(BINARIES)
ifeq ($(wildcard /tftpboot/speck),/tftpboot/speck)
	cp $(BINARIES) /tftpboot/speck/
endif

.PHONY: setup
setup:
	(test -d sys || mkdir sys)
	(test -d sys/kernel || mkdir sys/kernel)
	(test -d sys/bin || mkdir sys/bin)

.PHONY: clean
clean:
	rm -rf sys floppy.img win32/bochsout.txt
