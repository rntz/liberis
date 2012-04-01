#include "vm.h"

#define SHAPE(shape)                             \
    shape_t eris_shape_##shape = {               \
        .name = #shape                           \
    }

SHAPE(nil);
SHAPE(num);
SHAPE(proto);
SHAPE(closure);
SHAPE(c_closure);
SHAPE(string);
SHAPE(cons);
SHAPE(vec);
SHAPE(symbol);
SHAPE(cell);
