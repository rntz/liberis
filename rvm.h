#ifndef _RVM_H
#define _RVM_H_

#include <stddef.h>
#include <stdint.h>

/* Different instructions take different numbers of bytes to represent. However,
 * it behooves us to divide the instruction stream into fixed-size chunks such
 * that loading a single instruction (a) usually (for most instructions) takes
 * only a single load from memory, rather than one load per byte, and (b) these
 * accesses are properly aligned, so as not to incur a runtime penalty.
 *
 * On x86 and x86-64, the platforms we care about most, 4-byte chunks are
 * the choice that best satisfies these goals.
 */
typedef uint32_t rvm_instr_t;
typedef  uint8_t rvm_op_t;
typedef  uint8_t rvm_arg_t;
typedef uint16_t rvm_longarg_t;

typedef uintptr_t rvm_val_t;
typedef uint32_t rvm_int_t;
typedef uint8_t rvm_tag_t;

/* Generated tables. */
#include "enum_op.h"
#include "enum_tag.h"

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
    rvm_instr_t *code;
    uint8_t nargs;
    uint8_t nupvals;
} rvm_proto_t;

typedef struct {
    rvm_proto_t *proto;
    rvm_val_t *upvals;
} rvm_closure_t;

typedef struct {
    size_t len;
    const char data[1];
} rvm_string_t;

typedef struct {
    rvm_tag_t tag;
    union {
        rvm_closure_t closure;
        rvm_string_t string;
        rvm_val_t tuple[1];
    } data;
} rvm_object_t;

/* TODO: Perhaps this should have a better name. */
/* TODO: Distinguish between saved and active state structures of
 * interpreter. */
typedef struct {
    rvm_instr_t *pc;
    rvm_val_t *stack;
    size_t base;
    size_t stack_size;
    rvm_closure_t func;
    /* TODO: call/control stack. */
} rvm_state_t;

#endif