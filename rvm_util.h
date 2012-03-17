/* Not for public consumption. */
/* Useful macros and inline functions for the vm implementation. */
#ifndef _RVM_UTILS_H_
#define _RVM_UTILS_H_

#include "rvm.h"
#include "rvm_vm.h"
#include "rvm_runtime.h"
#include "misc.h"

/* Type checkers and converters. */
#define SHAPE_TYPE(shape) rvm_##shape##_t
#define SHAPE_TAG(shape) (&rvm_shape_##shape)

#define IS_INT(x) ((x) & 1)
#define IS_OBJ(x) (!IS_INT(x))

#define OBJ_VAL(obj) ((rvm_val_t) (obj))

#define VAL_OBJ val_obj
static inline rvm_obj_t *val_obj(rvm_val_t v)
{
    if (UNLIKELY(!IS_OBJ(v)))
        rvm_type_error("expected object, got int");
    assert (v);
    return (rvm_obj_t*) v;
}

#define OBJ_CONTENTS(shape, obj) ((SHAPE_TYPE(shape)*)((obj)+1))
#define CONTENTS_OBJ(val) (((rvm_obj_t*)(val)) - 1)

#define OBJ_ISA(shape, obj) ((obj)->tag == SHAPE_TAG(shape))
#define OBJ_IS_NIL(obj) OBJ_ISA(nil, obj)

#define VAL_IS_NIL val_is_nil
static inline bool val_is_nil(rvm_val_t v)
{
    return IS_OBJ(v) && OBJ_IS_NIL((rvm_obj_t*)v);
}

#define OBJ_CHECK_TAG(shape, obj) obj_check_tag(SHAPE_TAG(shape), (obj))
static inline rvm_obj_t *obj_check_tag(rvm_shape_t *tag, rvm_obj_t *obj)
{
    assert (obj);
    if (UNLIKELY(obj->tag != tag))
        rvm_type_error("expected %p, got %p", tag, obj->tag);
    return obj;
}

#define OBJ_TO(shape, obj) OBJ_CONTENTS(shape, OBJ_CHECK_TAG(shape, obj))
#define VAL_TO(shape, value) OBJ_TO(shape, VAL_OBJ(value))

#define OBJ_CLOSURE(o) OBJ_TO(closure, o)

#define VAL_CLOSURE(v) VAL_TO(closure, v)
#define VAL_CONS(v) VAL_TO(cons, v)
#define VAL_STRING(v) VAL_TO(string, v)


/* This is to be used only in cases where we statically know that v is a "cell".
 * TODO: explain cells somewhere in docs & reference here. */
static inline rvm_cell_t *get_cell(rvm_val_t v)
{
    assert (IS_OBJ(v));
    return OBJ_TO(cell, (rvm_obj_t*) v);
}

static inline rvm_val_t deref_cell(rvm_cell_t *g)
{
    if (UNLIKELY(!g->val)) {
        /* Cell is undefined. */
        /* TODO: print out symbol name. */
        rvm_die("reference to undefined cell");
    }
    return g->val;
}


/* Memory allocators. */
#define SHAPE_SIZE(shape, extra)                                \
    (sizeof(rvm_obj_t) + sizeof(SHAPE_TYPE(shape)) + (extra))

#define NELEM_SIZE(shape, elem_mem, nelems)             \
    (nelems * membersize(SHAPE_TYPE(shape), elem_mem[0]))

#define NEW_PLUS(shape, extra)                          \
    rvm_new(SHAPE_TAG(shape), SHAPE_SIZE(shape, extra))

#define NEW_WITH(shape, elem_mem, nelems)                       \
    NEW_PLUS(shape, NELEM_SIZE(shape, elem_mem, nelems))

#define NEW(shape) NEW_PLUS(shape, 0)

#define MAKE_PLUS(shape, extra) OBJ_CONTENTS(shape, NEW_PLUS(shape, extra))

#define MAKE_WITH(shape, elem_mem, nelems)                      \
    MAKE_PLUS(shape, NELEM_SIZE(shape, elem_mem, nelems))

#define MAKE(shape) MAKE_PLUS(shape, 0)

#define MAKE_CLOSURE(nupvals) MAKE_WITH(closure, upvals, nupvals)

#endif
