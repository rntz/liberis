#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>

#include "rvm.h"
#include "rvm_runtime.h"

void rvm_vdie(const char *fmt, va_list ap)
{
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    exit(1);
}

void rvm_die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    rvm_vdie(fmt, ap);
    va_end(ap);                 /* unreachable */
}

void rvm_type_error(char *x, ...)
{
    rvm_die("type error");
    (void) x;
}

void rvm_arity_error(char *x, ...)
{
    rvm_die("arity error");
    (void) x;
}

rvm_obj_t *rvm_new(rvm_shape_t *tag, size_t size)
{
    assert (size >= sizeof(rvm_obj_t)); /* sanity/precondition */
    rvm_obj_t *obj = malloc(size);
    if (!obj)
        rvm_die("OOM allocating object with tag %p", tag);
    assert (obj);
    obj->tag = tag;
    return obj;
}

void rvm_free(rvm_obj_t *obj)
{
    assert(obj);
    free(obj);
}
