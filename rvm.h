#ifndef _RVM_H_
#define _RVM_H_

#include <stddef.h>
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
typedef  uint32_t   rvm_instr_t;
typedef   uint8_t   rvm_op_t;
typedef   uint8_t   rvm_arg_t;
typedef  uint16_t   rvm_longarg_t;
typedef   int16_t   rvm_jump_offset_t;

typedef uintptr_t   rvm_val_t;
typedef  uint32_t   rvm_int_t;
typedef   uint8_t   rvm_tag_t;
typedef uintptr_t   rvm_hash_t;

typedef   uint8_t   rvm_reg_t;
typedef   uint8_t   rvm_nargs_t;
typedef   uint8_t   rvm_upval_t;

/* Generated tables. */
#include "enum_op.h"

/* The magic constants used here are obvious, but will change if instruction
 * encoding scheme changes. */
/* RVMI_ARG0 is used by instructions spanning multiple rvm_instr_ts. */
#define RVMI_OP(instr)          ((rvm_op_t)(instr))
#define RVMI_ARG0(instr)        ((rvm_arg_t)(instr))
#define RVMI_ARG1(instr)        ((rvm_arg_t)((instr) >> 8))
#define RVMI_ARG2(instr)        ((rvm_arg_t)((instr) >> 16))
#define RVMI_ARG3(instr)        ((rvm_arg_t)((instr) >> 24))
#define RVMI_LONGARG2(instr)    ((rvm_longarg_t)((instr) >> 16))


/* Data structures. */
typedef struct {
    const char *name;           /* a C string, null-byte and all. */
    /* TODO: gc information. */
} rvm_shape_t;

/* An object is a pointer to its shape, followed by a value of that shape. */
typedef struct {
    rvm_shape_t *tag;
    /* The value goes here. */
} rvm_obj_t;

/* For declaring shapes. */
#define SHAPE(name)                                     \
    extern rvm_shape_t rvm_shape_##name;                \
    typedef struct rvm_##name rvm_##name##_t;           \
    struct rvm_##name

/* Types after this point should only exist embedded inside of an rvm_obj_t. */
extern rvm_shape_t rvm_shape_nil;

SHAPE(proto) {
    rvm_instr_t *code;
    rvm_nargs_t num_args;
    rvm_upval_t num_upvals;
    bool variadic;
    rvm_proto_t *local_funcs[];
};

SHAPE(closure) {
    rvm_proto_t *proto;
    rvm_val_t upvals[];
};

/* TODO: decide re unicode & encoding stuff. */
SHAPE(string) {
    size_t len;
    const char data[];
};

SHAPE(cons) {
    rvm_val_t car;
    rvm_val_t cdr;
};

SHAPE(vec) {
    size_t len;
    rvm_val_t data[];
};

/* Symbols are just interned strings. */
extern rvm_shape_t rvm_shape_symbol;
typedef rvm_string_t rvm_symbol_t;

SHAPE(global) {
    /* If this is 0/NULL, the global is undefined. */
    rvm_val_t val;
    /* Information on where the global came from. */
    rvm_symbol_t *symbol;
};

/* clean up our macros */
#undef SHAPE


/* Data types below this line are not embedded inside an rvm_obj_t. */
typedef struct {
    rvm_instr_t *pc;
    rvm_closure_t *func;
} rvm_frame_t;

/* TODO: Perhaps this should have a better name. */
/* TODO: Distinguish between saved and active state structures of
 * interpreter. */
typedef struct {
    rvm_instr_t *pc;
    rvm_val_t *regs;
    rvm_frame_t *frames;        /* control/return stack */
    rvm_closure_t *func;
} rvm_state_t;

#endif
