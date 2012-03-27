#include <assert.h>
#include <string.h>

#include <eris/eris.h>

#include "misc.h"
#include "runtime.h"
#include "vm.h"
#include "vm_util.h"


/* Helper functions for call instructions. */
/* Does arity checking and conses excess variadic arguments. */
static inline
void do_precall(vm_state_t *S, closure_t *func,
                reg_t offset, nargs_t nargs)
{
    if (LIKELY(nargs == func->proto->num_args))
        return;

    if (LIKELY(func->proto->variadic)) {
        eris_die("variadic function calls unimplemented");
    }
    else {
        eris_arity_error("arity mismatch");
    }
    (void) S; (void) offset;    /* unused */
}

static inline
void do_call(vm_state_t *S, closure_t *func,
             reg_t offset, nargs_t nargs)
{
    do_precall(S, func, offset, nargs);

    /* Push return frame on control stack. */
    frame_t *frame = ++S->frames;
    frame->pc = S->pc;
    frame->func = S->func;

    /* Jump into the function. */
    S->func = func;
    S->pc = S->func->proto->code;
    S->regs += offset;
}

static inline
void do_tailcall(vm_state_t *S, closure_t *func,
                 reg_t offset, nargs_t nargs)
{
    do_precall(S, func, offset, nargs);

    /* Jump into the function. */
    S->func = func;
    S->pc = S->func->proto->code;
    /* Move down the arguments into appropriate slots. */
    memmove(S->regs, S->regs + offset, sizeof(val_t) * nargs);
}

static inline
void do_cond(vm_state_t *S, bool cond)
{
    /* The instruction after a conditional is required to be a jump. */
    assert (VM_OP(S->pc[1]) == OP_JUMP);

    if (cond)
        /* Skip next instruction. */
        S->pc += 2;
    else
        /* Make the following jump. (1 + ...) because the current pc is 1
         * behind that of the jump instruction.
         */
        S->pc += 1 + (jump_offset_t) VM_LONGARG2(*(S->pc + 1));
}


/* The main loop */

/* TODO: currently we do no cleaning-up of the stack, ever. unless we annotate
 * functions with live ranges for registers, this makes gc extra-conservative in
 * a very unpredictable way. highly undesirable, could cause long-lived garbage.
 */

void eris_vm_run(vm_state_t *state)
{
    vm_state_t S = *state;
#define REG(n)      (S.regs[n])

    /* The ((void) 0)s that you see in the following code are garbage to appease
     * the C99 spec, which allows only that a _statement_, not a _declaration_,
     * follow a label or case. */
  begin:
    (void) 0;
    const instr_t instr = *S.pc;

    /* Use these macros only if their value is to be used only once. */
#define OP   VM_OP(instr)
#define ARG1 VM_ARG1(instr)
#define ARG2 VM_ARG2(instr)
#define ARG3 VM_ARG3(instr)
#define LONGARG2 VM_LONGARG2(instr)
#define DEST REG(ARG1)

#define UPVAL(upval) (S.func->upvals[(upval)])
#define CELL(upval) (deref_cell(get_cell(UPVAL(upval))))

    /* TODO: order cases by frequency. */
    switch ((enum op) OP) {
      case OP_MOVE:
        DEST = REG(ARG2);
        ++S.pc;
        break;

      case OP_LOAD_INT:
        /* Tag the integer appropriately. */
        /* TODO: factor out into macros. */
        DEST = ((val_t)LONGARG2 << 1) | 1;
        ++S.pc;
        break;

      case OP_LOAD_UPVAL:
        DEST = UPVAL(ARG2);
        ++S.pc;
        break;

      case OP_LOAD_CELL:
        DEST = CELL(ARG2);
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
#define CELL_FUNC VAL_CLOSURE(CELL(ARG1))
#define REG_FUNC VAL_CLOSURE(REG(ARG1))

      case OP_CALL_CELL:
        do_call(&S, CELL_FUNC, ARG2, ARG3);
        break;

      case OP_CALL_REG:
        do_call(&S, REG_FUNC, ARG2, ARG3);
        break;

      case OP_TAILCALL_CELL:
        do_tailcall(&S, CELL_FUNC, ARG2, ARG3);
        break;

      case OP_TAILCALL_REG:
        do_tailcall(&S, REG_FUNC, ARG2, ARG3);
        break;


        /* Other instructions. */
      case OP_JUMP:
        /* NOT C99 SPEC: unsigned to signed integer conversion is
         * implementation-defined or may raise a signal when the unsigned value
         * is not representable in the signed target type. We depend on
         * 2's-complement representation, with this cast being a no-op.
         *
         * TODO: It should be possible to fix this by explicitly doing the
         * appropriate modulo arithmetic, and if gcc & clang are smart enough
         * this will compile into a nop on x86(-64). Should test this, though.
         */
        S.pc += (jump_offset_t) LONGARG2;
        break;

      case OP_RETURN: (void) 0;
        /* Put the return value where it ought to be. */
        REG(0) = REG(ARG1);
        frame_t *frame = S.frames--;
        /* We determine how far to pop the stack by looking at the argument
         * offset given in the CALL instruction that set up our frame.
         */
        S.regs -= VM_ARG2(*frame->pc);
        S.pc = frame->pc + 1;   /* +1 to skip past the call instr. */
        S.func = frame->func;
        break;

      case OP_IF:
        do_cond(&S, !VAL_IS_NIL(REG(ARG1)));
        break;

      case OP_IFNOT:
        do_cond(&S, VAL_IS_NIL(REG(ARG1)));
        break;

      case OP_CLOSE: (void) 0;
        arg_t which_func = ARG1;
        upval_t nupvals = ARG2;
        proto_t *proto = S.func->proto->local_funcs[which_func];
        closure_t *func = MAKE_CLOSURE(nupvals);
        func->proto = proto;

        /* TODO: load upvals into func. */

        assert(0);

      default:
#ifdef ERIS_RELEASE
        UNREACHABLE;
#endif
        eris_die("unrecognized or unimplemented opcode: %d", OP);
    }

    goto begin;
}

/* Don't put anything here. */
