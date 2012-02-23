#include "rvm_vm.h"

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
    (&objof(RVM_TAG_##tagname, (obj))->data.member)

#define VALDATA(tagname, member, value) OBJDATA(tagname, member, valobj(value))

#define VALTUPLE(v) ((rvm_val_t*)*VALDATA(TUPLE, tuple, v))
#define VALCLOSURE(v) VALDATA(CLOSURE, closure, v)
#define VALSTRING(v) VALDATA(STRING, string, v)


/* Useful stuff. */
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

      case RVM_OP_CALL: {
          rvm_val_t func_ref = S.func.upvals[ARG1];
          uint8_t arg0_reg = ARG2;
          uint8_t nargs = ARG3;
          rvm_closure_t *func = VALCLOSURE(VALTUPLE(func_ref)[0]);

          /* Set up frame on control stack. */
          rvm_frame_t *frame = ++S.frames;
          frame->pc = S.pc;
          frame->func = S.func;

          /* Munge our state. */
          S.func = *func;
          S.regs += arg0_reg;
          rvm_proto_t *proto = S.func.proto;
          S.pc = proto->code;
          if (nargs != proto->nargs) {
              /* SLOW PATH. */
              /* TODO: give gcc hint indicating this is unlikely. */
              if (proto->variadic) {
                  rvm_die("variadic function calls unimplemented");
              }
              rvm_arity_error("arity mismatch");
          }
      }
        break;

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
