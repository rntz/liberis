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

enum builtin_op {
#define BUILTIN(name, ...) BOP_##name,
#include "builtins.expando"
#undef BUILTIN
};

/* The magic constants used here are obvious, but will change if instruction
 * encoding scheme changes. */
/* VM_ARG0 is used by instructions spanning multiple instr_ts. */
#define VM_OP(instr)          ((op_t)(instr))
#define VM_ARG0(instr)        ((arg_t)(instr))
#define VM_ARG1(instr)        ((arg_t)((instr) >> 8))
#define VM_ARG2(instr)        ((arg_t)((instr) >> 16))
#define VM_ARG3(instr)        ((arg_t)((instr) >> 24))
#define VM_ARGN(instr, n)     ((arg_t)((instr) >> (8*(n))))
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
    union {
        struct {
            instr_t *ip;
            closure_t *func;
        } eris_call;
        struct {
            c_closure_t *func;
            size_t num_regs;
        } c_call;
    } data;
} frame_t;

typedef struct {
    instr_t *ip;
    /* Index into our frame on the stack of registers. This stack grows up. */
    val_t *regs;
    /* Stack of control/return frames. This stack grows down. Points to _our_
     * control frame, just below the frame of the function we return into. Note
     * that our control frame's IP only needs to be brought up-to-date
     * immediately before either:
     *
     * 1. a (non-TAIL) CALL instr (so that it knows where to return to)
     *
     * 2. the instruction triggers GC (so that precise GC based on IP works). of
     *    course, we handle this conservatively and update whenever we /might/
     *    trigger GC (eg. when we heap-allocate).
     */
    frame_t *frame;
    closure_t *func;
    eris_thread_t *thread;
} vm_state_t;


/* Interface to the vm. */
void eris_vm_run(vm_state_t *state);

#endif
