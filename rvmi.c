#include <stdio.h>
#include <string.h>

#include "rvm.h"
#include "rvm_vm.h"
#include "misc.h"

#define I1(OP, X) ((rvm_instr_t) (CAT(RVM_OP_,OP) ^ (X << 8)))
#define I2(OP, A, B) I1(OP, A ^ (B << 8))
#define I3(OP, A, B, C) I2(OP, A, B ^ (C << 8))

/* /\* foo *\/
 * rvm_instr_t foo_code[] = {
 *     I1(RETURN, 0)
 * };
 * 
 * rvm_proto_t foo_proto = {
 *     .code = foo_code,
 *     .num_args = 1,
 *     .num_upvals = 0,
 *     .variadic = false
 * };
 * 
 * rvm_object_t foo_object = {
 *     .tag = RVM_TAG_CLOSURE,
 *     .data.closure = {
 *         .proto = &foo_proto,
 *     },
 *     .data.closure.upvals = { 0 }
 * };
 * 
 * rvm_object_t foo_ref_object = {
 *     .tag = RVM_TAG_TUPLE,
 *     .data.tuple = { (rvm_val_t) &foo_object }
 * };
 * 
 * /\* bar *\/
 * rvm_instr_t bar_code[] = {
 *     I2(LOAD_INT, 1, 0xfeed),
 *     I3(CALL, 0, 0, 1),
 *     I1(RETURN, 0)
 * };
 * 
 * rvm_proto_t bar_proto = {
 *     .code = bar_code,
 *     .num_args = 0,
 *     .num_upvals = 1,
 *     .variadic = false
 * };
 * 
 * /\* rvm_val_t bar_upvals[] = {
 *  *     (rvm_val_t) &foo_ref_object
 *  * }; *\/
 * 
 * rvm_closure_t bar_closure = {
 *     .proto = &bar_proto,
 *     .upvals = {
 *         (rvm_val_t) &foo_ref_object
 *     }
 * }; */


void poison(void *x, uint32_t word, size_t size) {
    memset(x, (int) word, size);
    uint32_t *mem = x;
    for (size_t i = 0; i + sizeof(word) <= size; i += sizeof(word))
        *mem++ = word;
}

int main(int argc, char **argv)
{
    /* rvm_val_t stack[100];
     * rvm_frame_t cont[10]; */

    /* poison(stack, 0xdeadbeef, sizeof(stack));
     * poison(cont, 0xcafebabe, sizeof(cont));
     * 
     * rvm_state_t state = ((rvm_state_t) {
     *         .pc = bar_code,
     *         .regs = stack,
     *         .frames = cont,
     *         .func = &bar_closure
     * }); */

    /* rvm_run(&state); */

    return 0;
    (void) argc, (void) argv;
}
