EXES=rvmi

RVMI_LIBS=slz
RVMI_SOURCES=rvmi.c rvm_vm.c rvm_runtime.c rvm_shapes.c

HEADERS=rvm.h rvm_runtime.h rvm_util.h rvm_vm.h misc.h
MISC_FILES=README enum_op Makefile config.mk depclean genenum
TAR_FILES=$(MISC_FILES) $(HEADERS) $(RVMI_SOURCES)

# Make "all" default target.
.PHONY: all
all:

include config.mk

all: $(EXES)

# Real targets.
rvmi: $(RVMI_SOURCES:.c=.o)
rvmi: LIBS+=$(RVMI_LIBS)

# Tarballs
rvm.tar.gz: $(TAR_FILES)
rvm.tar.bz2: $(TAR_FILES)

# Disassembly targets.
ifneq (, $(filter rvmi.s rvmi.rodata,$(MAKECMDGOALS)))
CFLAGS+= -g
endif

rvmi.s: rvmi
	objdump -M intel -S -d $< > $@

rvmi.rodata: rvmi
	objdump --full-contents -j .rodata $< > $@


# Pattern rules
%.o: %.c flags
	@echo "   CC	$<"
	$(CC) $(CFLAGS) -c $< -o $@

$(EXES): %:
	@echo "   LD	$^"
	$(CCLD) $(LDFLAGS) -o $@ $^

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
	@echo ENUM_HEADERS="$(ENUM_HEADERS)" >> $@
	@md5sum Makefile >> $@


# Cleaning stuff.
CLEAN_RULES=nodeps clean pristine
.PHONY: $(CLEAN_RULES)

nodeps:
	./depclean

clean:
	find . -name '*.o' -delete
	rm -f $(EXES) rvm.tar.* rvmi.s rvmi.rodata

pristine: clean nodeps
	rm -f flags new_flags $(ENUM_HEADERS)


# Enum header file autogeneration.
ENUM_ROOTS=op
ENUM_INPUTS=$(addprefix enum_, $(ENUM_ROOTS))
ENUM_HEADERS=$(addsuffix .h, $(ENUM_INPUTS))

all: $(ENUM_HEADERS)

# default, assumes enumerations are 8-bit.
MAXENUMVAL=256

$(ENUM_HEADERS): enum_%.h: enum_% genenum
	@echo "   ENUM	$@"
	./genenum $(notdir $*) $(MAXENUMVAL) < $< > $@


# Automatic dependency generation.
# Complicated slightly by the presence of automatically-generated headers.

# Empty dep files indicate a deleted source file; we should get rid of them.
$(shell find . -name '*.dep' -empty -print0 | xargs -0 rm -f)

%.dep: %.c flags $(ENUM_HEADERS)
	@echo "   DEP	$<"
	set -e; $(CC) -MM -MT $< $(filter-out -pedantic,$(CFLAGS)) $< |\
	sed 's,\($*\)\.c *:,\1.o $@ :,' > $@

CFILES=$(shell find . -name '*.c')

# Only include dep files if not cleaning.
NODEP_RULES=$(CLEAN_RULES) rvm.tar.% uninstall

ifneq (,$(filter-out $(NODEP_RULES), $(MAKECMDGOALS)))
include $(CFILES:.c=.dep)
else ifeq (,$(MAKECMDGOALS))
include $(CFILES:.c=.dep)
else
endif
