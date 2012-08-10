#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include <eris/eris.h>

#include "misc.h"
#include "types.h"
#include "runtime.h"
#include "vm.h"

#define I1(OP, X) ((instr_t) (CAT(OP_,OP) ^ ((X) << 8)))
#define I2(OP, A, B) I1(OP, (A) ^ ((B) << 8))
#define I3(OP, A, B, C) I2(OP, A, (B) ^ ((C) << 8))

obj_t *make_cell(char *name, val_t v)
{
    cell_t *g;
    if (!new_cell(&g, NULL, NULL)) /* FIXME */
        abort();
    g->val = v;
    g->symbol = NULL;             /* TODO: use `name' for this. */
    (void) name;
    return CONTENTS_OBJ(g);
}


/* foo */
instr_t foo_code[] = {
    I1(RETURN, 0)
};

proto_t foo_proto = {
    .code = foo_code,
    .num_args = 1,
    .num_upvals = 0,
    .variadic = false,
};

struct {
    shape_t *tag;
    closure_t closure;
} foo = {
    .tag = SHAPE_TAG(closure),
    .closure = {
        .proto = &foo_proto,
    }
};

closure_t *make_foo(void) { return &foo.closure; }


/* bar */
instr_t bar_code[] = {
    I2(LOAD_INT, 1, 0xfeed),
    I3(CALL_CELL, 0, 1, 1),
    I1(RETURN, 1)
};

closure_t *make_bar(void)
{
    proto_t *proto;
    if (!new_proto(&proto, 1, NULL, NULL)) /* FIXME */
        abort();
    *proto = ((proto_t) {
            .code = bar_code,
            .num_args = 0,
            .num_upvals = 1,
            .variadic = false });

    closure_t *foo = make_foo();
    proto->local_funcs[0] = foo->proto;

    obj_t *gfoo = make_cell("foo", CONTENTS_VAL(foo));

    closure_t *bar;
    if (!new_closure(&bar, 1, NULL, NULL)) /* FIXME */
        abort();
    bar->proto = proto;
    bar->upvals[0] = OBJ_VAL(gfoo);
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
#define NUM_CONTS 20
    val_t stack[100];
    frame_t cont[NUM_CONTS];

    poison(stack, 0xdeadbeef, sizeof(stack));
    poison(cont, 0xcafebabe, sizeof(cont));

    closure_t *bar = make_bar();

    /* Initialize frame. */
    /* NUM_CONTS-2 to leave a poisoned frame at the top */
    frame_t *frame = &cont[NUM_CONTS-2];
    frame->tag = FRAME_CALL;
    frame->data.call.func = bar;

    vm_state_t state = ((vm_state_t) {
            .ip = bar_code,
            .regs = stack,
            .frame = frame,
            .func = bar
    });

    eris_vm_run(&state);

    return 0;
    (void) argc, (void) argv;
}
