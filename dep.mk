# Empty dep files indicate a deleted source file; we should get rid of them.
$(shell find . -name '*.dep' -empty -print0 | xargs -0 rm -f)

%.dep: %.c
	set -e; $(CC) -MM -MT $< $(filter-out -pedantic,$(CFLAGS)) $< | sed 's,\($*\)\.c *:,\1.o $@ :,' > $@

CFILES=$(shell find . -name '*.c')
include $(CFILES:.c=.dep)
