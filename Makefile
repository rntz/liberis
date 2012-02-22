.DEFAULT_GOAL := all

.PHONY: all nothing depclean

all: opcodes.h

nothing:
	@echo OK.

depclean:
	./depclean

# Actual targets.
include dep.mk

# Generated header file.
opcodes.h: opcodes genopcodes
	./genopcodes < $< > $@
