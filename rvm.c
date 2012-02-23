#include "rvm.h"

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

void rvm_vdie(const char *fmt, va_list ap)
{
    vfprintf(stderr, fmt, ap);
    exit(1);
}

void rvm_die(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    rvm_vdie(fmt, ap);
    va_end(ap);                 /* unreachable */
}

void rvm_type_error(char *x, ...)
{
    rvm_die("type error");
    (void) x;
}

void rvm_arity_error(char *x, ...)
{
    rvm_die("arity error");
    (void) x;
}

#define IS_INT(x) (x & 1)
#define IS_OBJ(x) (!IS_INT(x))

static inline rvm_object_t *object(rvm_val_t v)
{
    if (!IS_OBJ(v))
        rvm_type_error("expected object, got int");
    return (rvm_object_t*) v;
}

void rvm_run(rvm_state_t *state)
{
    rvm_state_t S = *state;
    rvm_val_t *regs = S.stack + S.base;
#define REG(n)      (regs + (n))
#define REGVAL(n)   (*REG(n))

  begin:
    (void)0;
    rvm_instr_t instr = *(S.pc++);

    /* Use these macros only if their value is to be used only once. */
#define OP   RVMI_OP(instr)
#define ARG1 RVMI_ARG1(instr)
#define ARG2 RVMI_ARG2(instr)
#define ARG3 RVMI_ARG3(instr)
#define LONGARG2 RVMI_LONGARG2(instr)
#define DEST REG(ARG1)

    switch (OP) {
      case RVM_OP_MOVE:
        *DEST = *REG(ARG2);
        break;

      case RVM_OP_LOAD_INT:
        /* Tag the integer appropriately. */
        *DEST = ((rvm_val_t)LONGARG2 << 1) | 1;
        break;

      case RVM_OP_LOAD_UPVAL:
        *DEST = S.func.upvals[ARG2];
        break;

      case RVM_OP_CALL:
        (void)0;
        rvm_val_t func_ref = S.func.upvals[ARG1];
        uint8_t arg0_reg = ARG2;
        uint8_t num_arg_regs = ARG3;
        object(func_ref);
        assert(0);
        /* XXX */
        (void) arg0_reg, (void) num_arg_regs;

      default:
        rvm_die("unrecognized or unimplemented opcode: %d", OP);
    }

    goto begin;
}

/* Don't put anything here. */
