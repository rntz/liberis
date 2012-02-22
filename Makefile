include dep.mk

# Generated header file.
opcodes.h: opcodes genopcodes
	./genopcodes < $< > $@