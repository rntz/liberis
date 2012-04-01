#ifndef _TYPES_H_
#define _TYPES_H_

#include <stdint.h>
#include <stdbool.h>

#include <gmp.h>

#include <eris/eris.h>

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

typedef uint8_t num_tag_t;
enum num_tag { SMALL_INT, SMALL_FLOAT, LARGE_RATIO, LARGE_FLOAT };

SHAPE(num) {
    num_tag_t tag;
    union {
        intptr_t small_int;
        double small_float;
        mpq_t large_ratio;
        mpf_t large_float;
    };
};

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

SHAPE(c_closure) {
    eris_c_func_t func;
    void *data;
    size_t num_upvals;
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

#endif
