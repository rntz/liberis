#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>

#include "runtime.h"

void eris_vdie(const char *fmt, va_list ap)
{
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

void eris_die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    eris_vdie(fmt, ap);
    va_end(ap);                 /* unreachable */
}

void eris_type_error(char *x, ...)
{
    eris_die("type error");
    (void) x;
}

void eris_arity_error(char *x, ...)
{
    eris_die("arity error");
    (void) x;
}

obj_t *eris_new(shape_t *tag, size_t size)
{
    assert (size >= sizeof(obj_t)); /* sanity/precondition */
    obj_t *obj = malloc(size);
    if (!obj)
        eris_die("OOM allocating object with tag %p", tag);
    assert (obj);
    obj->tag = tag;
    return obj;
}

void eris_free(obj_t *obj)
{
    assert(obj);
    free(obj);
}
