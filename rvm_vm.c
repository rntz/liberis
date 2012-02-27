#include <assert.h>
#include <string.h>

#include "rvm.h"
#include "rvm_vm.h"
#include "rvm_runtime.h"
#include "misc.h"

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

/* Mallocers. */
#define membersize(type, mem) sizeof(((type*)NULL)->mem)

#define NEW_PLUS(tag, mem, extra)                                       \
    (&rvm_new(TAG(tag), membersize(rvm_obj_t, data.mem) + (extra))->data.mem)

#define NEW(tag, mem) NEW_PLUS(tag, mem, 0)

#define NEW_WITH(tag, mem, elem_mem, nelems)                            \
    NEW_PLUS(tag, mem, nelems * membersize(rvm_obj_t, data.mem.elem_mem[0]))

#define NEW_CLOSURE(nupvals) NEW_WITH(CLOSURE, closure, upvals, nupvals)

#define IS_INT(x) (x & 1)
#define IS_OBJ(x) (!IS_INT(x))

#define OBJ_VAL(obj) ((rvm_val_t) (obj))

#define VAL_OBJ val_obj
static inline rvm_obj_t *val_obj(rvm_val_t v)
{
    if (UNLIKELY(!IS_OBJ(v)))
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
    if (UNLIKELY(obj->tag != tag))
        rvm_type_error("expected %d, got %d", tag, obj->tag);
    return obj;
}

#define OBJ_AS(tagname, member, obj) (&OBJ_CHECK_TAG(tagname, obj)->data.member)
#define VAL_AS(tagname, member, value) OBJ_AS(tagname, member, VAL_OBJ(value))

#define VAL_CLOSURE(v) VAL_AS(CLOSURE, closure, v)
#define VAL_CONS(v) VAL_AS(CONS, cons, v)
#define VAL_STRING(v) VAL_AS(STRING, string, v)

/* This is to be used only in cases where we statically know that v is a "global
 * cell". TODO: explain global cells somwhere in docs & reference here. */
static inline rvm_global_t *get_global(rvm_val_t v)
{
    assert (IS_OBJ(v));
    rvm_obj_t *obj = (rvm_obj_t*) v;
    assert (OBJ_ISA(GLOBAL, obj));
    return &obj->data.global;
}

static inline rvm_val_t deref_global(rvm_global_t *g)
{
    if (UNLIKELY(!g->val)) {
        /* Global is undefined. */
        /* TODO: print out symbol name. */
        rvm_die("reference to undefined global");
    }
    return g->val;
}


/* Helper functions for call instructions. */
/* Does arity checking and conses excess variadic arguments. */
static inline
void do_precall(rvm_state_t *S, rvm_closure_t *func,
                rvm_reg_t offset, rvm_nargs_t nargs)
{
    if (LIKELY(nargs == func->proto->num_args))
        return;

    if (LIKELY(func->proto->variadic)) {
        rvm_die("variadic function calls unimplemented");
    }
    else {
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
#define REG(n)      (S.regs[n])

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

#define UPVAL(upval) (S.func->upvals[(upval)])
#define GLOBAL(upval) (deref_global(get_global(UPVAL(upval))))

    /* TODO: order cases by frequency. */
    switch (OP) {
      case RVM_OP_MOVE:
        DEST = REG(ARG2);
        ++S.pc;
        break;

      case RVM_OP_LOAD_INT:
        /* Tag the integer appropriately. */
        /* TODO: factor out into macros. */
        DEST = ((rvm_val_t)LONGARG2 << 1) | 1;
        ++S.pc;
        break;

      case RVM_OP_LOAD_UPVAL:
        DEST = UPVAL(ARG2);
        ++S.pc;
        break;

      case RVM_OP_LOAD_GLOBAL:
        DEST = GLOBAL(ARG2);
        ++S.pc;
        break;


        /* Call instructions. */
        /* FORMAT OF CALL INSTR:
         *    OP (8 bits): the opcode
         *  ARG1 (8 bits): register or upval indicating function to call
         *  ARG2 (8 bits): first argument register
         *  ARG3 (8 bits): number of argument registers
         *
         *  So if ARG2 = 3 and ARG3 = 4, the arguments are in registers 3-6
         *  inclusive.
         *
         *  If OP is CALL or TAILCALL, ARG1 is an upval index. The upval does
         *  not refer to a function to call; rather it refers to a ref cell
         *  which refers to the function to call. Ideally, the upval should be
         *  statically guaranteed to be a ref cell, so tag-checking it should be
         *  unnecessary; for now, we do the check anyway. Checking that what the
         *  ref-cell points to is a function is, however, necessary.
         *
         *  If OP is CALL_REG or TAILCALL_REG, ARG1 is a register, which
         *  contains the function to be called. A type check is necessary as
         *  usual.
         *
         *  CALL_FUNC gets the closure being called for CALL and TAILCALL ops.
         *  CALL_REG_FUNC does the same for CALL_REG and TAILCALL_REG.
         */
#define CALL_FUNC VAL_CLOSURE(GLOBAL(ARG1))
#define CALL_REG_FUNC VAL_CLOSURE(REG(ARG1))

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
        REG(0) = REG(ARG1);
        rvm_frame_t *frame = S.frames--;
        /* We determine how far to pop the stack by looking at the argument
         * offset given in the CALL instruction that set up our frame.
         */
        S.regs -= RVMI_ARG2(*frame->pc);
        S.pc = frame->pc + 1;   /* +1 to skip past the call instr. */
        S.func = frame->func;
        break;

      case RVM_OP_IF:
        do_cond(&S, !VAL_IS_NIL(REG(ARG1)));
        break;

      case RVM_OP_IFNOT:
        do_cond(&S, VAL_IS_NIL(REG(ARG1)));
        break;

      case RVM_OP_CLOSE: (void) 0;
        rvm_arg_t which_func = ARG1;
        rvm_upval_t nupvals = ARG2;
        rvm_proto_t *proto = S.func->proto->local_funcs[which_func];
        rvm_closure_t *func = NEW_CLOSURE(nupvals);
        func->proto = proto;

        /* TODO: load upvals into func. */

        assert(0);

      default:
#ifdef RVM_RELEASE
        UNREACHABLE;
#endif
        rvm_die("unrecognized or unimplemented opcode: %d", OP);
    }

    goto begin;
}

/* Don't put anything here. */
