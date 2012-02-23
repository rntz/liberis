#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rvm_vm.h"
#include "misc.h"

void rvm_vdie(const char *fmt, va_list ap)
{
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
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


/* Type converters. */
#define IS_INT(x) (x & 1)
#define IS_OBJ(x) (!IS_INT(x))

static inline rvm_object_t *valobj(rvm_val_t v)
{
    if (!IS_OBJ(v))
        rvm_type_error("expected object, got int");
    assert (v);
    return (rvm_object_t*) v;
}

static inline rvm_object_t *objof(rvm_tag_t tag, rvm_object_t *obj)
{
    assert (obj);
    if (obj->tag != tag)
        rvm_type_error("expected %d, got %d", tag, obj->tag);
    return obj;
}

/* XXX: document */
#define OBJDATA(tagname, member, obj)                   \
    (&objof(CAT(RVM_TAG_,tagname), (obj))->data.member)

#define VALDATA(tagname, member, value) OBJDATA(tagname, member, valobj(value))

#define VALTUPLE(v) ((rvm_val_t*)*VALDATA(TUPLE, tuple, v))
#define VALCLOSURE(v) VALDATA(CLOSURE, closure, v)
#define VALSTRING(v) VALDATA(STRING, string, v)


/* Helper functions for call instructions. */
/* Does arity checking and conses excess variadic arguments. */
static inline
void rvm_precall(rvm_state_t *S, rvm_closure_t *func,
                 rvm_reg_t offset, rvm_nargs_t nargs)
{
    if (nargs == func->proto->num_args) return;
    /* TODO: give gcc hint indicating this is unlikely. */
    if (func->proto->variadic) {
        rvm_die("variadic function calls unimplemented");
    }
    else {
        /* SLOWER PATH. */
        /* TODO: give gcc hint indicating this is unlikely. */
        rvm_arity_error("arity mismatch");
    }
    (void) S; (void) offset;    /* unused */
}

static inline
void rvm_call(rvm_state_t *S, rvm_closure_t *func,
              rvm_reg_t offset, rvm_nargs_t nargs)
{
    rvm_precall(S, func, offset, nargs);

    /* Push return frame on control stack. */
    rvm_frame_t *frame = ++S->frames;
    frame->pc = S->pc;
    frame->func = S->func;

    /* Munge our state. */
    S->func = *func;
    S->regs += offset;
    S->pc = S->func.proto->code;
}

static inline
void rvm_tailcall(rvm_state_t *S, rvm_closure_t *func,
                  rvm_reg_t offset, rvm_nargs_t nargs)
{
    rvm_precall(S, func, offset, nargs);

    /* Munge our state. */
    S->func = *func;
    S->pc = S->func.proto->code;
    /* Move down the arguments into appropriate slots. */
    memmove(S->regs, S->regs + offset, sizeof(rvm_val_t) * nargs);
}


/* The main loop */

/* TODO: currently we do no cleaning-up of the stack, ever. unless we annotate
 * functions with live ranges for registers, this makes gc extra-conservative in
 * a very unpredictable way. highly undesirable, could cause long-lived garbage.
 */

void rvm_run(rvm_state_t *state)
{
    rvm_state_t S = *state;
#define REG(n)      (S.regs + (n))
#define REGVAL(n)   (*REG(n))

    /* The ((void) 0)s that you see in the following code are garbage to appease
     * the C99 spec, which allows only that a _statement_, not a _declaration_,
     * follow a label or case. */
  begin:
    (void) 0;
    const rvm_instr_t instr = *(S.pc++);

    /* Use these macros only if their value is to be used only once. */
#define OP   RVMI_OP(instr)
#define ARG1 RVMI_ARG1(instr)
#define ARG2 RVMI_ARG2(instr)
#define ARG3 RVMI_ARG3(instr)
#define LONGARG2 RVMI_LONGARG2(instr)
#define DEST REG(ARG1)

    switch (OP) {
        /* TODO: order by frequency. */
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


        /* Call instructions. */
        /* TODO: document these macros */
#define CALL_FUNC VALCLOSURE(VALTUPLE(S.func.upvals[ARG1])[0])
#define CALL_REG_FUNC VALCLOSURE(*REG(ARG1))

      case RVM_OP_CALL:
        rvm_call(&S, CALL_FUNC, ARG2, ARG3);
        break;

      case RVM_OP_CALL_REG:
        rvm_call(&S, CALL_REG_FUNC, ARG2, ARG3);
        break;

      case RVM_OP_TAILCALL:
        rvm_tailcall(&S, CALL_FUNC, ARG2, ARG3);
        break;

      case RVM_OP_TAILCALL_REG:
        rvm_tailcall(&S, CALL_REG_FUNC, ARG2, ARG3);
        break;


        /* Other instructions. */
      case RVM_OP_RETURN: {
          /* Put the return value where it ought to be. */
          *REG(0) = *REG(ARG1);
          rvm_frame_t *frame = S.frames--;
          S.pc = frame->pc;
          S.func = frame->func;
          /* We figure how far to pop the stack by looking at the argument
           * offset given in the CALL instruction that set up our frame, which
           * is the one immediately preceding the pc that we return to.
           */
          S.regs -= RVMI_ARG2(*(S.pc - 1));
      }
        break;

      default:
        rvm_die("unrecognized or unimplemented opcode: %d", OP);
    }

    goto begin;
}

/* Don't put anything here. */
