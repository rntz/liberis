#ifndef _VM_H_
#define _VM_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <eris/eris.h>

#include "types.h"

/* Opcode values. Note that this is not the type used to represent opcodes in
 * actual bytecode (that's op_t from types.h). It just defines some nice
 * integral constants for us, and lets us cast to this type when we want to be
 * sure we've handled all the branches in a switch.
 */
enum op {
    OP_MOVE, OP_LOAD_INT, OP_LOAD_UPVAL, OP_LOAD_CELL,

    /* unconditional control flow operators */
    OP_CALL_CELL, OP_CALL_REG, OP_TAILCALL_CELL, OP_TAILCALL_REG,
    OP_JUMP, OP_RETURN,

    /* conditional control flow operators */
    OP_IF, OP_IFNOT,

    /* miscellany */
    OP_CLOSE,
};

/* The magic constants used here are obvious, but will change if instruction
 * encoding scheme changes. */
/* VM_ARG0 is used by instructions spanning multiple instr_ts. */
#define VM_OP(instr)          ((op_t)(instr))
#define VM_ARG0(instr)        ((arg_t)(instr))
#define VM_ARG1(instr)        ((arg_t)((instr) >> 8))
#define VM_ARG2(instr)        ((arg_t)((instr) >> 16))
#define VM_ARG3(instr)        ((arg_t)((instr) >> 24))
#define VM_LONGARG2(instr)    ((longarg_t)((instr) >> 16))

/* Interface to the vm. */
void eris_vm_run(vm_state_t *state);

#endif
