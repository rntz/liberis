CFILES=$(shell find src/ -name '*.c')
HFILES=$(shell find src/ -name '*.h')
INCFILES=$(shell find include/ -name '*.h')
SOURCES=$(CFILES) $(HFILES) $(INCFILES) src/builtins.expando

MAKEFILES=Makefile main.mk config.mk

# Files included in distributed tarballs
TAR_FILES=$(SOURCES) include/README include/eris/builtins_pre \
	README $(MAKEFILES)

# Names of executables we generate
EXE_NAMES=rvmi

# Names of source files we generate
GENFILE_NAMES=include/eris/builtins.expando

# Libraries we depend on.
LIBS=gmp Judy

# Make "all" default target.
.PHONY: all
all:

INCLUDE_DIRS+= include/
include config.mk


# verbose flag
V?=0

ifeq (0,$(V))
QUIET:=@
else
QUIET:=
endif


# Cleaning stuff.
CLEAN_RULES=depclean objclean clean pristine
.PHONY: $(CLEAN_RULES)

depclean:
	rm -rf build/*/dep

objclean:
	rm -rf build/*/bin build/*/obj

clean:
	rm -rf build/
	rm -f eris.tar.*

pristine: clean
	rm -f TAGS


# If we're just cleaning, we can ignore everything else.
ifneq (,$(filter-out $(CLEAN_RULES), $(MAKECMDGOALS)))
include main.mk
else ifeq (,$(MAKECMDGOALS))
include main.mk
endif
