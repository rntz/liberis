/* Useful macros and inline functions for the vm implementation. */
#ifndef _VM_UTIL_H_
#define _VM_UTIL_H_

#include "misc.h"
#include "runtime.h"
#include "vm.h"

/* Type checkers and converters. */
#define SHAPE_TYPE(shape) shape##_t
#define SHAPE_TAG(shape) (&eris_shape_##shape)

#define OBJ_VAL(obj) ((val_t) (obj))
#define VAL_OBJ(val) ((obj_t*) val)

#define OBJ_CONTENTS(shape, obj) ((SHAPE_TYPE(shape)*)((obj)+1))
#define CONTENTS_OBJ(ptr) (((obj_t*)(ptr)) - 1)
#define CONTENTS_VAL(ptr) OBJ_VAL(CONTENTS_OBJ(ptr))

#define OBJ_ISA(shape, obj) ((obj)->tag == SHAPE_TAG(shape))
#define OBJ_IS_NIL(obj) OBJ_ISA(nil, obj)

#define VAL_IS_NIL(v) OBJ_IS_NIL(VAL_OBJ(v))

#define OBJ_CHECK_TAG(shape, obj) obj_check_tag(SHAPE_TAG(shape), (obj))
static inline obj_t *obj_check_tag(shape_t *tag, obj_t *obj)
{
    assert (obj);
    if (UNLIKELY(obj->tag != tag))
        eris_type_error("expected %p, got %p", tag, obj->tag);
    return obj;
}

#define OBJ_AS(shape, obj) OBJ_CONTENTS(shape, OBJ_CHECK_TAG(shape, obj))
#define VAL_AS(shape, value) OBJ_AS(shape, VAL_OBJ(value))

#define OBJ_AS_CLOSURE(o) OBJ_AS(closure, o)

#define VAL_AS_CLOSURE(v) VAL_AS(closure, v)
#define VAL_AS_CONS(v) VAL_AS(cons, v)
#define VAL_AS_STRING(v) VAL_AS(string, v)


/* This is to be used only in cases where we statically know that v is a "cell".
 * TODO: explain cells somewhere in docs & reference here. */
static inline cell_t *get_cell(val_t v)
{
    return OBJ_AS(cell, (obj_t*) v);
}

static inline val_t deref_cell(cell_t *g)
{
    if (UNLIKELY(!g->val)) {
        /* Cell is undefined. */
        /* TODO: print out symbol name. */
        eris_die("reference to undefined cell");
    }
    return g->val;
}


/* Memory allocators. */
#define SHAPE_SIZE(shape, extra)                                \
    (sizeof(obj_t) + sizeof(SHAPE_TYPE(shape)) + (extra))

#define NELEM_SIZE(shape, elem_mem, nelems)             \
    (nelems * membersize(SHAPE_TYPE(shape), elem_mem[0]))

#define NEW_PLUS(shape, extra)                          \
    eris_new(SHAPE_TAG(shape), SHAPE_SIZE(shape, extra))

#define NEW_WITH(shape, elem_mem, nelems)                       \
    NEW_PLUS(shape, NELEM_SIZE(shape, elem_mem, nelems))

#define NEW(shape) NEW_PLUS(shape, 0)

#define MAKE_PLUS(shape, extra) OBJ_CONTENTS(shape, NEW_PLUS(shape, extra))

#define MAKE_WITH(shape, elem_mem, nelems)                      \
    MAKE_PLUS(shape, NELEM_SIZE(shape, elem_mem, nelems))

#define MAKE(shape) MAKE_PLUS(shape, 0)

#define MAKE_CLOSURE(nupvals) MAKE_WITH(closure, upvals, nupvals)

#endif
