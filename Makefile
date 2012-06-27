SOURCES=$(shell find src/ -name '*.c')
MAKEFILES=Makefile main.mk config.mk

# Files included in distributed tarballs
TAR_FILES=$(SOURCES) $(shell find src/ -name '*.h') src/builtins.expando \
	include/README include/eris/builtins_pre \
	$(shell find include/ -name '*.h') \
	README $(MAKEFILES) depclean

# Names of executables we generate
EXE_NAMES=rvmi

# Names of source files we generate
GENFILE_NAMES=include/eris/builtins.expando

# Libraries we depend on.
LIBS=gmp slz Judy

# Make "all" default target.
.PHONY: all
all:

INCLUDE_DIRS+= include/
include config.mk


# Cleaning stuff.
CLEAN_RULES=nodeps clean pristine
.PHONY: $(CLEAN_RULES)

nodeps:
	./depclean

clean:
	rm -rf build/
	rm -f eris.tar.*

pristine: clean nodeps
	rm -f _MAKEFLAGS _NEW_MAKEFLAGS _TMP.mk


# We segregate compiles under different flags into different directories.
# Here be magic.

# We need to use FORCE rather than just declaring _MAKEFLAGS to be phony because
# phony targets are (it seems?) never considered up-to-date when used as
# dependencies. We want the recipe for _MAKEFLAGS to always run, but _MAKEFLAGS
# to be considered up-to-date as a dependency if it hasn't changed.
.PHONY: FORCE
FORCE:

_MAKEFLAGS: _NEW_MAKEFLAGS FORCE
	@{ test -f $@ && diff -q $@ $< >/dev/null; } || \
	{ echo "Flags and/or makefiles changed; remaking."; cp $< $@; }
	@rm $<

_NEW_MAKEFLAGS:
	@echo CC="$(CC)" > $@
	@echo CCLD="$(CCLD)" >> $@
	@echo CFLAGS="$(CFLAGS)" >> $@
	@echo LDFLAGS="$(LDFLAGS)" >> $@
	@echo LDLIBS="$(LDLIBS)" >> $@
	@echo GENFILE_NAMES="$(sort $(GENFILE_NAMES))" >> $@
	@sha1sum $(MAKEFILES) >> $@

# Hack to get main.mk not to load until _MAKEFLAGS is created.
_TMP.mk: _MAKEFLAGS
	@echo "include main.mk" > $@

# If we're just cleaning, no need to jump through all these hoops.
ifneq (,$(filter-out $(CLEAN_RULES), $(MAKECMDGOALS)))
include _TMP.mk
else ifeq (,$(MAKECMDGOALS))
include _TMP.mk
endif
