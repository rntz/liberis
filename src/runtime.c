#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>

#include "runtime.h"

obj_t *eris_new(eris_thread_t *T, void *F, shape_t *tag, size_t size)
{
    assert (size >= sizeof(obj_t)); /* sanity/precondition */
    obj_t *obj = malloc(size);
    if (!obj)
        eris_bug("OOM allocating object with tag %p", tag);
    assert (obj);
    obj->tag = tag;
    return obj;
    (void) T;
    (void) F;
}

void eris_free(eris_thread_t *T, void *F, obj_t *obj)
{
    assert(obj);
    free(obj);
    (void) T;
    (void) F;
}

void eris_type_error(eris_thread_t *T, void *F, char *fmt, ...)
{
    /* TODO */
    eris_bug("type error");
    (void) T;
    (void) F;
    (void) fmt;
}

void eris_arity_error(eris_thread_t *T, void *F, char *fmt, ...)
{
    /* TODO */
    eris_bug("arity error");
    (void) T;
    (void) F;
    (void) fmt;
}

void eris_vbug(const char *fmt, va_list ap)
{
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    abort();
}

void eris_bug(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    eris_vbug(fmt, ap);
    va_end(ap);                 /* unreachable */
}
