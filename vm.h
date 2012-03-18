#ifndef _ERIS_VM_H_
#define _ERIS_VM_H_

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include <eris.h>

#include "types.h"
#include "enum_op.h"

/* The magic constants used here are obvious, but will change if instruction
 * encoding scheme changes. */
/* VM_ARG0 is used by instructions spanning multiple instr_ts. */
#define VM_OP(instr)          ((op_t)(instr))
#define VM_ARG0(instr)        ((arg_t)(instr))
#define VM_ARG1(instr)        ((arg_t)((instr) >> 8))
#define VM_ARG2(instr)        ((arg_t)((instr) >> 16))
#define VM_ARG3(instr)        ((arg_t)((instr) >> 24))
#define VM_LONGARG2(instr)    ((longarg_t)((instr) >> 16))

/* Interface to the vm. */
void eris_vm_run(vm_state_t *state);

#endif
