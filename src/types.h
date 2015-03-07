#ifndef _TYPES_H_
#define _TYPES_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <gmp.h>

/* We use Judy arrays for various things. Not because we've actually determined
 * that we need the speed and memory characteristics of Judy arrays, just
 * because they present a nice, simple interface.
 */
#include <Judy.h>

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
typedef   int16_t   signed_longarg_t;

typedef uintptr_t   val_t;
typedef   uint8_t   tag_t;
typedef uintptr_t   hash_t;
typedef   uint8_t   builtin_op_t;

typedef   uint8_t   reg_t;
typedef   uint8_t   nargs_t;
typedef   uint8_t   upval_t;

/* Opcode values. Note that this is not the type used to represent opcodes in
 * actual bytecode (that's op_t, above). It just defines some nice integral
 * constants for us, and lets us cast to this type when we want to be sure we've
 * handled all the branches in a switch.
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


/* Eris object structures */
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

extern shape_t eris_shape_nil;
extern const val_t eris_nil;

/* TODO: complex numbers. */
typedef uint8_t num_tag_t;
enum num_tag { NUM_INTPTR, NUM_MPQ, NUM_DOUBLE };

SHAPE(num) {
    num_tag_t tag;
    union {
        intptr_t v_intptr;
        mpq_t v_mpq;
        double v_double;
    } data;
};

SHAPE(builtin) {
    builtin_op_t op;
    nargs_t num_args;
    bool variadic;
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
    val_t name;                 /* invariant: is a string */
    size_t num_upvals;
    val_t upvals[];
};

/* TODO: figure out unicode/encoding issues. */
SHAPE(string) {
    size_t len;
    const char data[];
};

/* TODO: Immutable sequence representation should be smarter.
 * At the very least, ropes, if not finger trees.
 */
SHAPE(seq) {
    size_t len;
    val_t data[];
};

/* NB. differ from seqs in tha they are mutable */
/* TODO: these should be extensible arrays with a capacity & size. */
SHAPE(vec) {
    size_t len;
    val_t data[];
};

SHAPE(symbol) {
    size_t len;
    const char data[];
};

SHAPE(cell) {
    /* If this is 0/NULL, the cell is undefined. */
    val_t val;
    /* Information on where the cell came from. */
    symbol_t *symbol;
};

/* clean up our macros */
#undef SHAPE


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
        } call;
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


/* Runtime data structures */
struct eris_vm {
    /* TODO: GC metadata. */
    /* Judy array of interned symbols. */
    Pvoid_t symbols;
    val_t symbol_t;         /* the "t" symbol, used as a canonical true value */
    /* Linked list of threads. */
    eris_thread_t *threads;
};

struct eris_thread {
    eris_vm_t *vm;
    /* Whether a frame has been created on this thread and is in use. */
    bool in_use;
    /* `regs' points to bottom of register stack. */
    val_t *regs;
    /* `frames' points to top of frame stack. dereferencing it is disallowed. */
    void *frames;
    /* TODO: GC metadata (eg. size of register & frame stacks). */
    /* Next thread on thread list. */
    eris_thread_t *next;
};

struct eris_frame {
    /* Points to the bottom of our chunk of the register stack. */
    val_t *regs;
    /* How many registers are we using? Needed for push/pop/etc and for GC. */
    size_t num_regs;
    /* Points to our frame on the control stack, which is just below the frame
     * we return into. */
    frame_t *frame;
    eris_thread_t *thread;
    /* FIXME: need tailcall info */
};


#endif
