# make.inc

export VERSION = 0.2.0

#--------------------------------------
# defaults / auto-configuring guesses

arch=i386
PREFIX=
POSTFIX=

ifeq ($(arch),i386)
# check for cygwin
ifeq ($(wildcard /bin/cygwin1.dll),/bin/cygwin1.dll)
	# cygwin - i386-elf cross compiler
	PREFIX=i386-elf-
endif
endif

#--------------------------------------
CC=$(PREFIX)gcc$(POSTFIX)
LD = $(PREFIX)ld$(POSTFIX)
AR = $(PREFIX)ar$(POSTFIX)
STRIP = $(PREFIX)strip$(POSTFIX)
RANLIB = $(PREFIX)ranlib$(POSTFIX)

INCLUDE = -I $(srcroot)/include
CFLAGS = -O3 -Wall -Werror -fno-builtin $(INCLUDE)
ASFLAGS = $(INCLUDE)

#--------------------------------------
# include the local overrides

ifeq ($(wildcard $(srcroot)/config),$(srcroot)/config)
	include $(srcroot)/config
endif
