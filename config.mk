# Defaults.
CC=gcc
CFLAGS+= -std=c99 -pedantic -Wall -Wextra -Werror
LDFLAGS+=

# Debug build.
CFLAGS+= -ggdb3 -O0

# Release build.
# CFLAGS+= -O3 -DNDEBUG
