.PHONY: all
all:

include config.mk

# Real targets
EXES=rvmi
all: $(EXES)

RVMI_SOURCES=rvmi rvm
rvmi: $(addsuffix .o, $(RVMI_SOURCES))

# Pattern rules
%.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

$(EXES): %:
	$(CC) $(LDFLAGS) -o $@ $^


# Cleaning stuff.
.PHONY: depclean clean pristine

depclean:
	./depclean

clean:
	rm -f *.o

pristine: clean depclean
	rm -f $(ENUM_HEADERS)


# Enum header file autogeneration.
ENUM_ROOTS=tag op
ENUM_INPUTS=$(addprefix enum_, $(ENUM_ROOTS))
ENUM_HEADERS=$(addsuffix .h, $(ENUM_INPUTS))

all: headers
.PHONY: headers
headers: $(ENUM_HEADERS)

MAXENUMVAL=256			#default.

$(ENUM_HEADERS): enum_%.h: enum_% genenum
	./genenum $(notdir $*) $(MAXENUMVAL) < $< > $@


# Automatic dependency generation.
# Complicated slightly by the presence of automatically-generated headers.

# Empty dep files indicate a deleted source file; we should get rid of them.
$(shell find . -name '*.dep' -empty -print0 | xargs -0 rm -f)

%.dep: %.c $(ENUM_HEADERS)
	set -e; $(CC) -MM -MT $< $(filter-out -pedantic,$(CFLAGS)) $< | sed 's,\($*\)\.c *:,\1.o $@ :,' > $@

CFILES=$(shell find . -name '*.c')

# Only include dep files if not cleaning.
ifneq (,$(filter-out depclean clean pristine, $(MAKECMDGOALS)))
include $(CFILES:.c=.dep)
else ifeq (,$(MAKECMDGOALS))
include $(CFILES:.c=.dep)
else
endif
