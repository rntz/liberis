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


/* Type converters and other conveniences. */
#define TAG(tagname) CAT(RVM_TAG_,tagname)

#define IS_INT(x) (x & 1)
#define IS_OBJ(x) (!IS_INT(x))

#define OBJ_VAL(obj) ((rvm_val_t) (obj))

#define VAL_OBJ val_obj
static inline rvm_obj_t *val_obj(rvm_val_t v)
{
    if (!IS_OBJ(v))
        rvm_type_error("expected object, got int");
    assert (v);
    return (rvm_obj_t*) v;
}

#define OBJ_ISA(tagname, obj) ((obj)->tag == TAG(tagname))
#define OBJ_IS_NIL(obj) OBJ_ISA(NIL, obj)

#define VAL_IS_NIL val_is_nil
static inline bool val_is_nil(rvm_val_t v)
{
    return IS_OBJ(v) && OBJ_IS_NIL((rvm_obj_t*)v);
}

#define OBJ_CHECK_TAG(tag, obj) obj_check_tag(CAT(RVM_TAG_, tag), (obj))
static inline rvm_obj_t *obj_check_tag(rvm_tag_t tag, rvm_obj_t *obj)
{
    assert (obj);
    if (obj->tag != tag)
        rvm_type_error("expected %d, got %d", tag, obj->tag);
    return obj;
}

#define OBJ_AS(tagname, member, obj) (&OBJ_CHECK_TAG(tagname, obj)->data.member)
#define VAL_AS(tagname, member, value) OBJ_AS(tagname, member, VAL_OBJ(value))

#define VAL_CLOSURE(v) VAL_AS(CLOSURE, closure, v)
#define VAL_CONS(v) VAL_AS(CONS, cons, v)
#define VAL_REF(v) VAL_AS(REF, ref, v)
#define VAL_STRING(v) VAL_AS(STRING, string, v)


/* Helper functions for call instructions. */
/* Does arity checking and conses excess variadic arguments. */
static inline
void do_precall(rvm_state_t *S, rvm_closure_t *func,
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
void do_call(rvm_state_t *S, rvm_closure_t *func,
             rvm_reg_t offset, rvm_nargs_t nargs)
{
    do_precall(S, func, offset, nargs);

    /* Push return frame on control stack. */
    rvm_frame_t *frame = ++S->frames;
    frame->pc = S->pc;
    frame->func = S->func;

    /* Jump into the function. */
    S->func = func;
    S->regs += offset;
    S->pc = S->func->proto->code;
}

static inline
void do_tailcall(rvm_state_t *S, rvm_closure_t *func,
                 rvm_reg_t offset, rvm_nargs_t nargs)
{
    do_precall(S, func, offset, nargs);

    /* Jump into the function. */
    S->func = func;
    S->pc = S->func->proto->code;
    /* Move down the arguments into appropriate slots. */
    memmove(S->regs, S->regs + offset, sizeof(rvm_val_t) * nargs);
}

static inline
void do_cond(rvm_state_t *S, bool cond)
{
    /* The instruction after a conditional is required to be a jump. */
    assert (RVMI_OP(S->pc[1]) == RVM_OP_JUMP);

    if (cond)
        /* Skip next instruction. */
        S->pc += 2;
    else
        /* Make the following jump. (1 + ...) because the current pc is 1
         * behind that of the jump instruction.
         */
        S->pc += 1 + (rvm_jump_offset_t) RVMI_LONGARG2(*(S->pc + 1));
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
    const rvm_instr_t instr = *S.pc;

    /* Use these macros only if their value is to be used only once. */
#define OP   RVMI_OP(instr)
#define ARG1 RVMI_ARG1(instr)
#define ARG2 RVMI_ARG2(instr)
#define ARG3 RVMI_ARG3(instr)
#define LONGARG2 RVMI_LONGARG2(instr)
#define DEST REG(ARG1)

    /* TODO: order cases by frequency. */
    switch (OP) {
      case RVM_OP_MOVE:
        *DEST = *REG(ARG2);
        ++S.pc;
        break;

      case RVM_OP_LOAD_INT:
        /* Tag the integer appropriately. */
        /* TODO: factor out into macros. */
        *DEST = ((rvm_val_t)LONGARG2 << 1) | 1;
        ++S.pc;
        break;

      case RVM_OP_LOAD_UPVAL:
        *DEST = S.func->upvals[ARG2];
        ++S.pc;
        break;


        /* Call instructions. */
        /* TODO: document these macros */
#define CALL_FUNC VAL_CLOSURE(*VAL_REF(S.func->upvals[ARG1]))
#define CALL_REG_FUNC VAL_CLOSURE(*REG(ARG1))

      case RVM_OP_CALL:
        do_call(&S, CALL_FUNC, ARG2, ARG3);
        break;

      case RVM_OP_CALL_REG:
        do_call(&S, CALL_REG_FUNC, ARG2, ARG3);
        break;

      case RVM_OP_TAILCALL:
        do_tailcall(&S, CALL_FUNC, ARG2, ARG3);
        break;

      case RVM_OP_TAILCALL_REG:
        do_tailcall(&S, CALL_REG_FUNC, ARG2, ARG3);
        break;


        /* Other instructions. */
      case RVM_OP_JUMP:
        /* NOT C99 SPEC: unsigned to signed integer conversion is
         * implementation-defined or may raise a signal when the unsigned value
         * is not representable in the signed target type. We depend on
         * 2's-complement representation, with this cast being a no-op.
         *
         * TODO: It should be possible to fix this by explicitly doing the
         * appropriate modulo arithmetic, and if gcc & clang are smart enough
         * this will compile into a nop on x86(-64). Should test this, though.
         */
        S.pc += (rvm_jump_offset_t) LONGARG2;
        break;

      case RVM_OP_RETURN: (void) 0;
        /* Put the return value where it ought to be. */
        *REG(0) = *REG(ARG1);
        rvm_frame_t *frame = S.frames--;
        /* We determine how far to pop the stack by looking at the argument
         * offset given in the CALL instruction that set up our frame.
         */
        S.regs -= RVMI_ARG2(*frame->pc);
        S.pc = frame->pc + 1;   /* +1 to skip past the call instr. */
        S.func = frame->func;
        break;

      case RVM_OP_IF:
        do_cond(&S, !VAL_IS_NIL(*REG(ARG1)));
        break;

      case RVM_OP_IFNOT:
        do_cond (&S, VAL_IS_NIL(*REG(ARG1)));
        break;

      default:
        rvm_die("unrecognized or unimplemented opcode: %d", OP);
    }

    goto begin;
}

/* Don't put anything here. */
