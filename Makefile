# Makefile

srcroot = .
include make.inc

# directories that produce binaries
SUBSYSTEMS = kernel testmod

SUBDIRS = $(SUBSYSTEMS) bin
SUBDIRS_SETUP := $(foreach subdir,$(SUBDIRS),$(subdir).setup)
SUBDIRS_CLEAN := $(foreach subdir,$(SUBDIRS),$(subdir).clean)

.PHONY: all
all: bin

.PHONY: $(SUBSYSTEMS)
$(SUBSYSTEMS): setup
	$(MAKE) -C $@

.PHONY: bin
bin: $(SUBSYSTEMS)
	$(MAKE) -C bin

.PHONY: setup
setup: $(SUBDIRS_SETUP) include/version.h

.PHONY: $(SUBDIRS_SETUP)
$(SUBDIRS_SETUP):
	$(MAKE) -C $(basename $@) setup

.PHONY: clean
clean: $(SUBDIRS_CLEAN)
	rm -f bochs.out include/version.h hardcopy.ps

.PHONY: $(SUBDIRS_CLEAN)
$(SUBDIRS_CLEAN):
	$(MAKE) -C $(basename $@) clean

.PHONY: include/version.h
include/version.h:
	@echo "#ifndef _VERSION_H_" > include/version.h
	@echo "#define _VERSION_H_" >> include/version.h

	@echo "#define SPECK_VERSION \"${VERSION}\"" >> include/version.h
	@echo "#define SPECK_ARCH \"$(arch)\"" >> include/version.h
	@echo "#define SPECK_BUILD_DATE \"$(shell date)\"" >> include/version.h
	@echo "#define SPECK_BUILD_USER \"$(shell whoami)\"" >> include/version.h
ifeq ($(shell uname),Linux)
	@echo "#define SPECK_BUILD_HOST \"$(shell hostname --fqdn)\"" >> include/version.h
else
	@echo "#define SPECK_BUILD_HOST \"$(shell hostname)\"" >> include/version.h
endif

	@echo "#endif" >> include/version.h

.PHONY: winbochs
winbochs:
ifeq ($(wildcard /bin/cygwin1.dll),/bin/cygwin1.dll)
	(cd bin/i386/win32; "C:/Program Files/bochs/bochs.exe")
else
	@echo You are not running cygwin. On a Unix box, set up your .bochsrc
	@echo to point to bin/i386/floppy.img and run bochs.
endif

.PHONY: hardcopy
hardcopy:
	enscript -r -2 -E --color -o hardcopy.ps `find . -name '*.[chS]' -o -name \
		'.asm' -o -name 'Makefile' -o -name 'make.inc' | grep -v '.svn'`
