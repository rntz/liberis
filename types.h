#ifndef _ERIS_TYPES_H_
#define _ERIS_TYPES_H_

#include <eris.h>

#include <stdint.h>
#include <stdbool.h>

/* Different instructions take different numbers of bytes to represent. However,
 * it behooves us to divide the instruction stream into fixed-size chunks such
 * that loading a single instruction (a) usually (for most instructions) takes
 * only a single load from memory, rather than one load per byte, and (b) these
 * accesses are properly aligned, so as not to incur a runtime penalty.
 *
 * On x86 and x86-64, the platforms we care about most, 4-byte chunks are
 * the choice that best satisfies these goals.
 */
typedef  uint32_t   instr_t;
typedef   uint8_t   op_t;
typedef   uint8_t   arg_t;
typedef  uint16_t   longarg_t;
typedef   int16_t   jump_offset_t;

typedef uintptr_t   val_t;
typedef  uint32_t   int_t;
typedef   uint8_t   tag_t;
typedef uintptr_t   hash_t;

typedef   uint8_t   reg_t;
typedef   uint8_t   nargs_t;
typedef   uint8_t   upval_t;


/* Data structures. */
typedef struct {
    const char *name;           /* a C string, null-byte and all. */
    /* TODO: gc information. */
} shape_t;

/* An object is a pointer to its shape, followed by a value of that shape. */
typedef struct {
    shape_t *tag;
    /* The value goes here. */
} obj_t;

/* For declaring shapes. */
#define SHAPE(name)                             \
    extern shape_t eris_shape_##name;           \
    typedef struct name name##_t;               \
    struct name


/* Types after this point should only exist embedded inside of an obj_t. */
extern shape_t eris_shape_nil;

SHAPE(proto) {
    instr_t *code;
    nargs_t num_args;
    upval_t num_upvals;
    bool variadic;
    proto_t *local_funcs[];
};

SHAPE(closure) {
    proto_t *proto;
    val_t upvals[];
};

/* TODO: decide re unicode & encoding stuff. */
SHAPE(string) {
    size_t len;
    const char data[];
};

SHAPE(cons) {
    val_t car;
    val_t cdr;
};

SHAPE(vec) {
    size_t len;
    val_t data[];
};

/* Symbols are just interned strings. */
extern shape_t eris_shape_symbol;
typedef string_t symbol_t;

SHAPE(cell) {
    /* If this is 0/NULL, the cell is undefined. */
    val_t val;
    /* Information on where the cell came from. */
    symbol_t *symbol;
};

/* clean up our macros */
#undef SHAPE


/* Data types below this line are not embedded inside an obj_t. */
typedef struct {
    instr_t *pc;
    closure_t *func;
} frame_t;

/* corresponding typedef is in eris.h */
struct eris_state {
    instr_t *pc;
    val_t *regs;
    frame_t *frames;        /* control/return stack */
    closure_t *func;
};

#endif
