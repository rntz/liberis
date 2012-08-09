#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stddef.h>

#include "runtime.h"

bool eris_new(obj_t **out,
              eris_thread_t *thread, frame_t *frame,
              shape_t *tag, size_t size)
{
    assert (size >= sizeof(obj_t)); /* sanity/precondition */
    obj_t *obj = malloc(size);
    if (!obj) {
        assert(0 && "malloc failed"); /* FIXME: remove */
        return false;
    }
    assert (obj);
    obj->tag = tag;
    *out = obj;
    return true;

    (void) thread; (void) frame;
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
