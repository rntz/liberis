/* Useful macros and inline functions for the vm implementation. */
#ifndef _VM_H_
#define _VM_H_

#include "misc.h"
#include "runtime.h"
#include "types.h"

/* The magic constants used here are obvious, but will change if instruction
 * encoding scheme changes. */
/* VM_ARG0 is used by instructions spanning multiple instr_ts. */
/* FIXME: what the hell does casting an integer to a shorter width do?
 * is this defined by C spec? */
#define VM_OP(instr)          ((op_t)(instr))
#define VM_ARG0(instr)        ((arg_t)(instr))
#define VM_ARG1(instr)        ((arg_t)((instr) >> 8))
#define VM_ARG2(instr)        ((arg_t)((instr) >> 16))
#define VM_ARG3(instr)        ((arg_t)((instr) >> 24))
#define VM_ARGN(instr, n)     ((arg_t)((instr) >> (8*(n))))
#define VM_LONGARG(instr)     ((longarg_t)((instr) >> 16))

/* NOT C99 SPEC: unsigned to signed integer conversion is implementation-defined
 * or may raise a signal when the unsigned value is not representable in the
 * signed target type. We depend on 2's-complement representation, with this
 * cast being a no-op.
 *
 * TODO: It should be possible to fix this by explicitly doing the appropriate
 * modulo arithmetic, and if gcc & clang are smart enough this will compile into
 * a nop on x86(-64). Should test this, though.
 */
#define VM_SIGNED_LONGARG(instr) ((signed_longarg_t) VM_LONGARG(instr))

/* Type checkers and converters.
 *
 * Some of these are uppercase even though they are functions. This is as a
 * reminder to people using the code, in case we decide to make them macros in
 * the future.
 *
 * Where possible, we make these functions, because:
 * - It aids debugging
 * - It makes compiler errors more helpful
 */
#define SHAPE_TYPE(shape) shape##_t
#define SHAPE_TAG(shape) (&eris_shape_##shape)

/* TODO: rename these so it's clearer what they do. */

static inline
val_t OBJ_VAL(obj_t *o) { return (val_t) o; }

static inline
obj_t *VAL_OBJ(val_t v){ return (obj_t*) v; }

#define VAL_CONTENTS(shape, val) OBJ_CONTENTS(shape, VAL_OBJ(val))
#define OBJ_CONTENTS(shape, obj) ((SHAPE_TYPE(shape)*)obj_contents(obj))
static inline
void *obj_contents(obj_t *obj) { return obj + 1; }

static inline
obj_t *CONTENTS_OBJ(void *ptr) { return ((obj_t*)(ptr)) - 1; }

static inline
val_t CONTENTS_VAL(void *ptr) { return OBJ_VAL(CONTENTS_OBJ(ptr)); }

static inline
bool VAL_IS_NIL(val_t v) { return v == eris_nil; }

static inline
bool OBJ_IS_NIL(obj_t *o) { return VAL_IS_NIL(OBJ_VAL(o)); }

#define VAL_ISA(shape, val) OBJ_ISA(shape, VAL_OBJ(val))
#define OBJ_ISA(shape, obj) obj_isa(SHAPE_TAG(shape), obj)
static inline
bool obj_isa(shape_t *tag, obj_t *obj) { return obj->tag == tag; }

/* WTB: macro-defining macros */
#define MAKE_SHAPE_GETTER(shape)                                        \
    static inline                                                       \
    bool get_obj_as_##shape(obj_t *_obj, SHAPE_TYPE(shape) **_out) {    \
        if (LIKELY(OBJ_ISA(shape, _obj))) {                             \
            *_out = OBJ_CONTENTS(shape, _obj);                          \
            return true;                                                \
        }                                                               \
        return false;                                                   \
    }

MAKE_SHAPE_GETTER(num)
MAKE_SHAPE_GETTER(builtin)
MAKE_SHAPE_GETTER(proto)
MAKE_SHAPE_GETTER(closure)
MAKE_SHAPE_GETTER(c_closure)
MAKE_SHAPE_GETTER(string)
MAKE_SHAPE_GETTER(seq)
MAKE_SHAPE_GETTER(vec)
MAKE_SHAPE_GETTER(symbol)
MAKE_SHAPE_GETTER(cell)

#undef MAKE_SHAPE_GETTER

#define OBJ_AS(shape, obj, out) get_obj_as_##shape((obj), (out))
#define VAL_AS(shape, val, out) OBJ_AS(shape, VAL_OBJ(val), out)


/* This is to be used only in cases where we statically know that v is a "cell".
 * TODO: explain cells somewhere in docs & reference here. */
static inline cell_t *get_cell(val_t v)
{
    assert (VAL_ISA(cell, v));
    return VAL_CONTENTS(cell, v);
}

static inline val_t deref_cell(cell_t *g)
{
    if (UNLIKELY(!g->val)) {
        /* Cell is undefined. */
        /* TODO: print out symbol name. */
        eris_bug("reference to undefined cell");
    }
    return g->val;
}


/* Memory allocators. */
#define SHAPE_SIZE(shape) (sizeof(obj_t) + sizeof(SHAPE_TYPE(shape)))

#define NELEM_SIZE(shape, elem_mem, nelems)                     \
    (nelems * membersize(SHAPE_TYPE(shape), elem_mem[0]))

#define SHAPE_SIZE_WITH(shape, elem_mem, nelems)                \
    (SHAPE_SIZE(shape) + NELEM_SIZE(shape, elem_mem, nelems))

/* #define NEW_PLUS(out, shape, extra)                              \
 *     eris_new(SHAPE_TAG(shape), SHAPE_SIZE(shape) + (extra))
 *
 * #define NEW_WITH(shape, elem_mem, nelems)                       \
 *     NEW_PLUS(shape, NELEM_SIZE(shape, elem_mem, nelems))
 *
 * #define NEW(shape) NEW_PLUS(shape, 0)
 *
 * #define MAKE_PLUS(shape, extra) OBJ_CONTENTS(shape, NEW_PLUS(shape, extra))
 *
 * #define MAKE_WITH(shape, elem_mem, nelems)                      \
 *     MAKE_PLUS(shape, NELEM_SIZE(shape, elem_mem, nelems))
 *
 * #define MAKE(shape) MAKE_PLUS(shape, 0)
 *
 * #define MAKE_CLOSURE(nupvals) MAKE_WITH(closure, upvals, nupvals)
 * #define MAKE_SEQ(nelems) MAKE_WITH(seq, data, nelems) */

/* Allocators for specific types. */
#define MAKE_ALLOCATOR_(shape, size, ...)                               \
    static inline                                                       \
    bool new_##shape(SHAPE_TYPE(shape) **out, __VA_ARGS__               \
                     eris_thread_t *thread,                             \
                     frame_t *frame)                                    \
    {                                                                   \
        obj_t *obj;                                                     \
        if (LIKELY(eris_new(&obj, thread, frame, SHAPE_TAG(shape), size))) { \
            *out = OBJ_CONTENTS(shape, obj);                            \
            return true;                                                \
        }                                                               \
        return false;                                                   \
    }

#define MAKE_ALLOCATOR(shape) MAKE_ALLOCATOR_(shape, SHAPE_SIZE(shape),)
#define MAKE_ALLOCATOR_NELEMS(shape, elem_mem) MAKE_ALLOCATOR_( \
        shape,                                                  \
        SHAPE_SIZE(shape) + nelems * membersize(                \
            SHAPE_TYPE(shape), elem_mem[0]),                    \
        size_t nelems,)

MAKE_ALLOCATOR(num)
MAKE_ALLOCATOR(builtin)
MAKE_ALLOCATOR_NELEMS(proto, local_funcs)
MAKE_ALLOCATOR_NELEMS(closure, upvals)
MAKE_ALLOCATOR_NELEMS(c_closure, upvals)
MAKE_ALLOCATOR_NELEMS(string, data)
MAKE_ALLOCATOR_NELEMS(seq, data)
MAKE_ALLOCATOR_NELEMS(vec, data)
MAKE_ALLOCATOR_NELEMS(symbol, data)
MAKE_ALLOCATOR(cell)

#undef MAKE_ALLOCATOR_NELEMS
#undef MAKE_ALLOCATOR
#undef MAKE_ALLOCATOR_

#endif
