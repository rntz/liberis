#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "misc.h"
#include "rvm.h"
#include "rvm_runtime.h"
#include "rvm_vm.h"
#include "rvm_util.h"

#define I1(OP, X) ((rvm_instr_t) (CAT(RVM_OP_,OP) ^ ((X) << 8)))
#define I2(OP, A, B) I1(OP, (A) ^ ((B) << 8))
#define I3(OP, A, B, C) I2(OP, A, (B) ^ ((C) << 8))

rvm_obj_t *make_global(char *name, rvm_val_t v)
{
    rvm_global_t *g = MAKE(global);
    g->val = v;
    g->symbol = NULL;             /* TODO: use `name' for this. */
    (void) name;
    return CONTENTS_OBJ(g);
}


/* foo */
rvm_instr_t foo_code[] = {
    I1(RETURN, 0)
};

rvm_proto_t foo_proto = {
    .code = foo_code,
    .num_args = 1,
    .num_upvals = 0,
    .variadic = false,
};

struct {
    rvm_shape_t *tag;
    rvm_closure_t closure;
} foo = {
    .tag = SHAPE_TAG(closure),
    .closure = {
        .proto = &foo_proto,
    }
};

rvm_obj_t *make_foo(void) { return CONTENTS_OBJ(&foo.closure); }


/* bar */
rvm_instr_t bar_code[] = {
    I2(LOAD_INT, 1, 0xfeed),
    I3(CALL, 0, 0, 1),
    I1(RETURN, 0)
};

rvm_obj_t *make_bar(void)
{
    rvm_proto_t *proto = MAKE_WITH(proto, local_funcs, 1);
    *proto = ((rvm_proto_t) {
            .code = bar_code,
            .num_args = 0,
            .num_upvals = 1,
            .variadic = false });

    rvm_obj_t *foo = make_foo();
    proto->local_funcs[0] = OBJ_CLOSURE(foo)->proto;

    rvm_obj_t *gfoo = make_global("foo", OBJ_VAL(foo));

    rvm_closure_t *bar = MAKE_CLOSURE(1);
    bar->proto = proto;
    bar->upvals[0] = OBJ_VAL(gfoo);
    return CONTENTS_OBJ(bar);
}


void poison(void *x, uint32_t word, size_t size) {
    memset(x, (int) word, size);
    uint32_t *mem = x;
    for (size_t i = 0; i + sizeof(word) <= size; i += sizeof(word))
        *mem++ = word;
}

int main(int argc, char **argv)
{
    rvm_val_t stack[100];
    rvm_frame_t cont[10];

    poison(stack, 0xdeadbeef, sizeof(stack));
    poison(cont, 0xcafebabe, sizeof(cont));

    rvm_obj_t *bar = make_bar();

    rvm_state_t state = ((rvm_state_t) {
            .pc = bar_code,
            .regs = stack,
            .frames = cont,
            .func = OBJ_CLOSURE(bar)
    });

    rvm_run(&state);

    return 0;
    (void) argc, (void) argv;
}
