#ifndef _RVM_H
#define _RVM_H_

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
typedef uint8_t rvm_opcode_t;
typedef uint8_t rvm_arg_t;
typedef uint8_t rvm_longarg_t;

/* Generated table of instr opcodes. */
#include "opcodes.h"

/* The magic constants used here are obvious, but will change if instruction
 * encoding scheme changes. */
/* RVM_INSTR_ARG0 is used by instructions spanning multiple rvm_instr_ts. */
#define RVM_INSTR_OPCODE(instr)     ((rvm_opcode_t)(instr))
#define RVM_INSTR_ARG0(instr)       ((rvm_arg_t)(instr))
#define RVM_INSTR_ARG1(instr)       ((rvm_arg_t)((instr) >> 8))
#define RVM_INSTR_ARG2(instr)       ((rvm_arg_t)((instr) >> 16))
#define RVM_INSTR_ARG3(instr)       ((rvm_arg_t)((instr) >> 24))
#define RVM_INSTR_LONGARG2(instr)   ((rvm_longarg_t)((instr) >> 16))

#endif
