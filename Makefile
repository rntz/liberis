.PHONY: all FORCE
all:
FORCE:

include config.mk

EXES=rvmi
all: $(EXES)

RVMI_SOURCES=rvmi rvm_vm rvm_runtime rvm_shapes
rvmi: $(addsuffix .o, $(RVMI_SOURCES))

# Pattern rules
%.o: %.c flags
	@echo "  CC	$<"
ifdef VERBOSE
	@$(CC) $(CFLAGS) -c $< -o $@
else
	$(CC) $(CFLAGS) -c $< -o $@
endif

$(EXES): %:
	@echo "  LD	$^"
ifdef VERBOSE
	@$(CCLD) $(LDFLAGS) -o $@ $^
else
	$(CCLD) $(LDFLAGS) -o $@ $^
endif

# Other miscellaneous rules
.PHONY: remake
remake: clean
	make all

# Used to force recompile if we change flags or makefiles.
flags: new_flags FORCE
	@echo "  FLAGS"
	@{ test -f $@ && diff -q $@ $< >/dev/null; } || \
	{ echo "Flags and makefiles changed; remaking."; cp $< $@; }
	@rm new_flags

new_flags:
	@echo CC="$(CC)" > $@
	@echo CFLAGS="$(CFLAGS)" >> $@
	@echo LDFLAGS="$(LDFLAGS)" >> $@
	@echo ENUM_HEADERS="$(ENUM_HEADERS)" >> $@
	@md5sum Makefile config.mk >> $@

# Disassembly targets.
ifeq (rvmi.s, $(MAKECMDGOALS))
CFLAGS+= -g
endif

rvmi.s: rvmi
	objdump -M intel -S -d $< > $@

rvmi.rodata: rvmi
	objdump --full-contents -j .rodata $< > $@


# Cleaning stuff.
.PHONY: depclean clean pristine

depclean:
	./depclean

clean:
	rm -f $(EXES) $(ENUM_HEADERS) *.o

pristine: clean depclean
	rm -f flags new_flags rvmi.s rvmi.rodata


# Enum header file autogeneration.
ENUM_ROOTS=op
ENUM_INPUTS=$(addprefix enum_, $(ENUM_ROOTS))
ENUM_HEADERS=$(addsuffix .h, $(ENUM_INPUTS))

.PHONY: headers
all: headers
headers: $(ENUM_HEADERS)

# default, assumes enumerations are 8-bit.
MAXENUMVAL=256

$(ENUM_HEADERS): enum_%.h: enum_% genenum
	@echo "  ENUM	'$@'"
ifdef VERBOSE
	@./genenum $(notdir $*) $(MAXENUMVAL) < $< > $@
else
	./genenum $(notdir $*) $(MAXENUMVAL) < $< > $@
endif


# Automatic dependency generation.
# Complicated slightly by the presence of automatically-generated headers.

# Empty dep files indicate a deleted source file; we should get rid of them.
$(shell find . -name '*.dep' -empty -print0 | xargs -0 rm -f)

%.dep: %.c flags $(ENUM_HEADERS)
	@echo "  DEP	$<"
	@set -e; $(CC) -MM -MT $< $(filter-out -pedantic,$(CFLAGS)) $< |\
	sed 's,\($*\)\.c *:,\1.o $@ :,' > $@

CFILES=$(shell find . -name '*.c')

# Only include dep files if not cleaning.
ifneq (,$(filter-out depclean clean pristine, $(MAKECMDGOALS)))
include $(CFILES:.c=.dep)
else ifeq (,$(MAKECMDGOALS))
include $(CFILES:.c=.dep)
else
endif
