SOURCES=$(shell find src/ -name '*.c')
MAKEFILES=Makefile main.mk config.mk

# Files included in distributed tarballs
TAR_FILES=$(SOURCES) $(shell find src/ -name '*.h') src/builtins.expando \
	include/README include/eris/builtins_pre \
	$(shell find include/ -name '*.h') \
	README $(MAKEFILES)

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


# If we're just cleaning, we can ignore everything else.
ifneq (,$(filter-out $(CLEAN_RULES), $(MAKECMDGOALS)))
include main.mk
else ifeq (,$(MAKECMDGOALS))
include main.mk
endif
