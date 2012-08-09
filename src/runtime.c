#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>

#include "runtime.h"

obj_t *eris_new(shape_t *tag, size_t size)
{
    assert (size >= sizeof(obj_t)); /* sanity/precondition */
    obj_t *obj = malloc(size);
    if (!obj)
        eris_bug("OOM allocating object with tag %p", tag);
    assert (obj);
    obj->tag = tag;
    return obj;
}

void eris_free(obj_t *obj)
{
    assert(obj);
    free(obj);
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

void eris_type_error(char *x, ...)
{
    eris_bug("type error");
    (void) x;
}

void eris_arity_error(char *x, ...)
{
    eris_bug("arity error");
    (void) x;
}
