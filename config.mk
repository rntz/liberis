# Defaults.
CC=gcc
CCLD=$(CC)
CFLAGS+= -std=c99 -Wall -Wextra -Werror -pipe
LDFLAGS+=

CFLAGS_DEBUG= -O0 -ggdb3
CFLAGS_RELEASE= -O3 -fomit-frame-pointer -DNDEBUG -DRVM_RELEASE
# feel free to mess around with this.
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


# For building with clang. Notably, don't have to change any compilation flags.
CC=clang


# Just for fun, if you want to see the various ways I'm violating the c99 spec.
#CFLAGS+= -pedantic -Wno-error
