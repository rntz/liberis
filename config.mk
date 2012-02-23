# Defaults.
CC=gcc
CFLAGS+= -std=c99 -Wall -Wextra -Werror
LDFLAGS+=

# Debug build.
CFLAGS+= -ggdb3 -O0

# Release build.
# CFLAGS+= -O3 -DNDEBUG


# For building with clang. Notably, don't have to change any compilation flags.
CC=clang


# Just for fun, if you want to see the various ways I'm violating the c99 spec.
#CFLAGS+= -pedantic -Wno-error
