# Directories to install to
ROOT=
PREFIX=$(ROOT)/usr/local
BIN=$(PREFIX)/bin
LIB=$(PREFIX)/lib
INCLUDE=$(PREFIX)/include

# Defaults.
CC=gcc
CPP=$(CC) -E
CCLD=$(CC)
CFLAGS+= -std=c99 -Wall -Wextra -Werror -pipe
# LIBS is defined in Makefile.
LDLIBS+= $(addprefix -l,$(LIBS))

# Directories to search for things.
INCLUDE_DIRS+= $(INCLUDE)
LIB_DIRS+= $(LIB)
CFLAGS+=$(addprefix -I,$(INCLUDE_DIRS))
LDFLAGS+=$(addprefix -L,$(LIB_DIRS))


# Release vs. debugging compile flags
ifeq (gcc,$(CC))
CFLAGS_DEBUG= -O0 -ggdb3
else
CFLAGS_DEBUG= -O0 -g
endif
CFLAGS_RELEASE= -O3 -fomit-frame-pointer -DNDEBUG -DERIS_RELEASE
# feel free to mess around with this one.
CFLAGS_CUSTOM=

# Default to debug.
ifeq (,$(MODE))
MODE=debug
endif

ifeq (debug,$(MODE))
CFLAGS+= $(CFLAGS_DEBUG)
else ifeq (release,$(MODE))
CFLAGS+= $(CFLAGS_RELEASE)
else ifeq (custom,$(MODE))
CFLAGS+= $(CFLAGS_CUSTOM)
else
$(error "unknown build mode: $(MODE)")
endif

BUILD_NAME=$(MODE)-$(CC)


# Uncomment the following to build with clang. clang produces nicer error
# messages, but forfeits the ability to use macros inside gdb.
#CC=clang


# Just for fun, if you want to see the various ways I'm violating the c99 spec.
#CFLAGS+= -pedantic -Wno-error
