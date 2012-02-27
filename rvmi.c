#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "misc.h"
#include "rvm.h"
#include "rvm_runtime.h"
#include "rvm_vm.h"

#define I1(OP, X) ((rvm_instr_t) (CAT(RVM_OP_,OP) ^ (X << 8)))
#define I2(OP, A, B) I1(OP, A ^ (B << 8))
#define I3(OP, A, B, C) I2(OP, A, B ^ (C << 8))

rvm_obj_t *make_global(char *name, rvm_val_t v)
{
    rvm_obj_t *g = rvm_new(RVM_TAG_GLOBAL, sizeof(rvm_global_t));
    g->data.global.val = v;
    g->data.global.symbol = NULL; /* TODO: use `name' for this */
    (void) name;
    return g;
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

rvm_obj_t foo_object = {
    .tag = RVM_TAG_CLOSURE,
    .data.closure = {
        .proto = &foo_proto,
    },
};

rvm_obj_t *make_foo(void) { return &foo_object; }


/* bar */
rvm_instr_t bar_code[] = {
    I2(LOAD_INT, 1, 0xfeed),
    I3(CALL, 0, 0, 1),
    I1(RETURN, 0)
};

rvm_obj_t *make_bar(void)
{
    rvm_proto_t *proto =
        malloc(sizeof(rvm_proto_t) + sizeof(rvm_proto_t*) * 1);
    assert (proto);
    *proto = ((rvm_proto_t) {
            .code = bar_code,
            .num_args = 0,
            .num_upvals = 1,
            .variadic = false });

    rvm_obj_t *foo = make_foo();
    proto->local_funcs[0] = foo->data.closure.proto;

    rvm_obj_t *gfoo = make_global("foo", (rvm_val_t) foo);

    rvm_obj_t *bar = rvm_new(RVM_TAG_CLOSURE,
                             sizeof(rvm_closure_t) +
                             sizeof(rvm_val_t) * 1);
    bar->data.closure.proto = proto;
    bar->data.closure.upvals[0] = (rvm_val_t) gfoo;
    return bar;
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
            .func = &bar->data.closure
    });

    rvm_run(&state);

    return 0;
    (void) argc, (void) argv;
}
