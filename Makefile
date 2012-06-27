EXES=rvmi
SOURCES=rvmi.c runtime.c shapes.c vm.c

TAR_FILES=$(SOURCES) \
	misc.h runtime.h types.h vm.h vm_util.h portability.h api.h \
	builtins.expando \
	include/eris/eris.h include/eris/portability.h \
	README include/README Makefile config.mk depclean \
	include/eris/builtins_pre

# Generated header files.
GENFILES=include/eris/builtins.expando

# Libraries we depend on.
LIBS=gmp slz Judy

# Make "all" default target.
.PHONY: all
all: $(EXES) $(GENFILES)

include config.mk

# Necessary additional configuration.
INCLUDE_DIRS+= include/


# Real targets.
rvmi: $(SOURCES:.c=.o)

# Tarballs
eris.tar.gz: $(TAR_FILES)
eris.tar.bz2: $(TAR_FILES)

# Autogenerated header files
include/eris/builtins.expando: builtins.expando include/eris/builtins_pre flags
	@echo "   GEN	$@"
	cat include/eris/builtins_pre > $@
	$(CPP) $(CFLAGS) -D'BUILTIN(x,...)=ERIS_BUILTIN(x)' -o - - < $< |\
	    sed '/^#\|^$$/d' >> $@

# Disassembly targets.
ifneq (, $(filter %.s %.rodata,$(MAKECMDGOALS)))
CFLAGS+= -g
endif

OD=objdump
ODFLAGS=-dSr

vm.s: vm.o
	@echo "   OBJDUMP	$<"
	$(OD) $(ODFLAGS) $< > $@

vm.rodata: vm.o
	@echo "   DUMP RODATA	$<"
	$(OD) $(ODFLAGS) --full-contents -j .rodata $< > $@

rvmi.s: rvmi
	@echo "   OBJDUMP	$<"
	$(OD) $(ODFLAGS) $< > $@

rvmi.rodata: rvmi
	@echo "   DUMP RODATA	$<"
	$(OD) $(ODFLAGS) --full-contents -j .rodata $< > $@


# Pattern rules
%.o: %.c flags
	@echo "   CC	$<"
	$(CC) $(CFLAGS) -c $< -o $@

$(EXES): %:
	@echo "   LD	$^"
	$(CCLD) $(LDFLAGS) -o $@ $^ $(LDLIBS)

%.tar.gz:
	@echo "   TAR	$@"
	ln -sf ./ $*
	tar czf $@ $(addprefix $*/,$^)
	rm $*

%.tar.bz2:
	@echo "   TAR	$@"
	ln -sf ./ $*
	tar cjf $@ $(addprefix $*/,$^)
	rm $*


# Used to force recompile if we change flags or makefiles.
.PHONY: FORCE
FORCE:

flags: new_flags FORCE
	@{ test -f $@ && diff -q $@ $< >/dev/null; } || \
	{ echo "Flags and makefiles changed; remaking."; cp $< $@; }
	@rm new_flags

new_flags:
	@echo CC="$(CC)" > $@
	@echo CCLD="$(CCLD)" >> $@
	@echo CFLAGS="$(CFLAGS)" >> $@
	@echo LDFLAGS="$(LDFLAGS)" >> $@
	@echo GENFILES="$(sort $(GENFILES))" >> $@
	@md5sum Makefile >> $@


# Cleaning stuff.
CLEAN_RULES=nodeps clean pristine
.PHONY: $(CLEAN_RULES)

nodeps:
	./depclean

clean:
	find . -name '*.o' -delete
	rm -f $(EXES) eris.tar.* rvmi.s rvmi.rodata

pristine: clean nodeps
	rm -f flags new_flags $(GENFILES)


# Automatic dependency generation.
# Complicated slightly by the presence of automatically-generated headers.

# Empty dep files indicate a deleted source file; we should get rid of them.
$(shell find . -name '*.dep' -empty -print0 | xargs -0 rm -f)

%.dep: %.c flags $(GENFILES)
	@echo "   DEP	$<"
	set -e; $(CC) -MM -MT $< $(filter-out -pedantic,$(CFLAGS)) $< |\
	sed 's,\($*\)\.c *:,\1.o $@ :,' > $@

CFILES=$(shell find . -name '*.c')

# Only include dep files if not cleaning.
NODEP_RULES=$(CLEAN_RULES) eris.tar.% uninstall

ifneq (,$(filter-out $(NODEP_RULES), $(MAKECMDGOALS)))
include $(CFILES:.c=.dep)
else ifeq (,$(MAKECMDGOALS))
include $(CFILES:.c=.dep)
else
endif
