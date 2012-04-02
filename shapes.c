#include "vm.h"

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
SHAPE(cons);
SHAPE(vec);
SHAPE(symbol);
SHAPE(cell);

/* Statically allocated values. */
obj_t eris_nil_obj = { .tag = &eris_shape_nil };
val_t eris_nil = (val_t) &eris_nil_obj;
