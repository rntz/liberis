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

    /* TODO: exceptions */
    /* /\* exceptional control flow operators *\/
     * OP_RAISE,
     * /\* NB. must precede an OP_CALL_* instr. *\/
     * OP_HANDLE, */

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


/* VM state structures */
typedef uint8_t frame_tag_t;
enum frame_tag {
    FRAME_CALL, FRAME_C_CALL,
    /* TODO: exceptions */
    /* FRAME_HANDLE, */
};

typedef struct {
    frame_tag_t tag;
    instr_t *pc;
    closure_t *func;
} call_frame_t;

typedef struct {
    frame_tag_t tag;
} c_call_frame_t;

typedef struct {
    instr_t *pc;
    /* Index into our frame on the stack of registers. This stack grows up. */
    val_t *regs;
    /* Stack of control/return frames. This stack grows down. Points to the tag
     * of the last frame pushed (the frame we return into).
     */
    void *frames;
    closure_t *func;
} vm_state_t;


/* Interface to the vm. */
void eris_vm_run(vm_state_t *state);

#endif
