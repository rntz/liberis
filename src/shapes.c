#include "types.h"

#define SHAPE(shape)                             \
    shape_t eris_shape_##shape = {               \
        .name = #shape                           \
    }

SHAPE(nil);
SHAPE(num);
SHAPE(builtin);
SHAPE(proto);
SHAPE(closure);
SHAPE(c_closure);
SHAPE(string);
SHAPE(seq);
SHAPE(vec);
SHAPE(symbol);
SHAPE(cell);

/* Statically allocated values. */
const obj_t eris_nil_obj = { .tag = &eris_shape_nil };
const val_t eris_nil = (val_t) &eris_nil_obj;
