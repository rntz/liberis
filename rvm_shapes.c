#include "rvm.h"

#define SHAPE(shape)                             \
    rvm_shape_t rvm_shape_##shape = {            \
        .name = #shape                           \
    }

SHAPE(nil);
SHAPE(proto);
SHAPE(closure);
SHAPE(string);
SHAPE(cons);
SHAPE(vec);
SHAPE(symbol);
SHAPE(cell);
