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
void do_precall(vm_state_t *S, closure_t *func, reg_t offset, nargs_t nargs)
{
    if (LIKELY(nargs == func->proto->num_args))
        return;

    if (LIKELY(func->proto->variadic)) {
        eris_die("variadic function calls unimplemented"); /* TODO */
    }
    else {
        eris_arity_error("arity mismatch");
    }
    (void) S; (void) offset;    /* unused */
}

static inline
void do_builtin(vm_state_t *S, obj_t *obj, reg_t offset, nargs_t nargs)
{
    assert (OBJ_ISA(builtin, obj));

    builtin_t *builtin = OBJ_CONTENTS(builtin, obj);
    if (UNLIKELY(nargs != builtin->num_args) &&
        (UNLIKELY(!builtin->variadic) || UNLIKELY(nargs < builtin->num_args)))
    {
        /* TODO: better error message */
        eris_arity_error("builtin");
    }

    switch (builtin->op) {
#define BUILTIN(name, num_args, variadic, ...)          \
        case CAT(BOP_,name): { __VA_ARGS__ } break;
#define NARGS nargs
#define ARG(i) S->regs[offset+(i)]
#define DEST S->regs[offset]
            /* TODO: better error messages */
#define ARITY_ERROR() eris_arity_error("builtin")
#define TYPE_ERROR() eris_type_error("builtin")
#define THREAD S->thread
#define UNIMPLEMENTED eris_die("builtin %u unimplemented", builtin->op);

#include "builtins.expando"

#undef UNIMPLEMENTED
#undef TYPE_ERROR
#undef ARITY_ERROR
#undef DEST
#undef ARG
#undef NARGS
#undef BUILTIN

      default: IMPOSSIBLE("unrecognized builtin: %u", builtin->op);
    }
    (void) nargs;            /* unused due to unimplemented variadic builtins */
}

/* Returns true iff we should next execute an OP_RETURN to simulate a
 * tailcall. */
static inline
bool do_call(vm_state_t *S,
             bool tail_call,
             val_t funcv,
             reg_t offset,
             nargs_t nargs)
{
    obj_t *obj = VAL_OBJ(funcv);
    shape_t *tag = obj->tag;

    /* Calling closures */
    if (LIKELY(tag == SHAPE_TAG(closure))) {
        closure_t *func = OBJ_CONTENTS(closure, obj);
        do_precall(S, func, offset, nargs);

        /* TODO: think very hard about what happens on control stack
         * overflow. */

        if (!tail_call) {
            /* Update our IP on control stack, so callee returns correctly. */
            S->frame->ip = S->ip;

            /* Push callee's stack frame. Remember, control stack grows down. */
            --S->frame;
            S->frame->tag = FRAME_CALL;
            /* No need to set frame's IP; callee will do that as necessary.
             * frame's func gets set unconditionally, below. */

            /* Shift our view of the register stack so our args are in the right
             * place. */
            S->regs += offset;
        }
        else {
            /* No need to update frame's IP; callee will do that as necessary.
             * No need to update frame's tag; already has appropriate value.
             * frame's func gets set unconditionally, below.
             */

            /* Move down arguments into appropriate slots. */
            memmove(S->regs, S->regs + offset, sizeof(val_t) * nargs);
        }

        /* Update frame. */
        S->frame->func = func;

        /* Jump into the function. */
        S->func = func;
        S->ip = S->func->proto->code;

        return false;            /* no OP_RETURN needed */
    }
    /* Calling builtins */
    else if (LIKELY(tag == SHAPE_TAG(builtin))) {
        do_builtin(S, obj, offset, nargs);
        /* "Tail calling" a built-in is emulated using OP_RETURN. */
        return tail_call;
    }
    /* Calling C closures */
    else if (LIKELY(tag == SHAPE_TAG(c_closure))) {
        assert(0 && "unimplemented"); /* TODO */
        /* "Tail calling" a C closure is emulated using OP_RETURN. */
        return tail_call;
    }
    /* Calling everything else */
    else {
        eris_type_error("invalid function object");
    }

    UNREACHABLE;
}

static inline
void do_cond(vm_state_t *S, bool cond)
{
    /* The instruction after a conditional is required to be a jump. */
    assert (VM_OP(S->ip[1]) == OP_JUMP);

    if (cond)
        /* Skip next instruction. */
        S->ip += 2;
    else
        /* Make the following jump. (1 + ...) because the current IP is 1
         * behind that of the jump instruction.
         */
        S->ip += 1 + (jump_offset_t) VM_LONGARG2(*(S->ip + 1));
}


/* The main loop */

/* TODO: currently we do no cleaning-up of the stack, ever. unless we annotate
 * functions with live ranges for registers, this makes gc extra-conservative in
 * a very unpredictable way. highly undesirable, could cause long-lived garbage.
 */

/* TODO: Use computed gotos if available. See femtolisp's flisp.c for a good way
 * to do this.
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
    const instr_t instr = *S.ip;

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
        ++S.ip;
        break;

      case OP_LOAD_INT: {
          /* Allocating; update control frame IP. */
          S.frame->ip = S.ip;
          obj_t *obj = NEW(num);
          num_t *num = OBJ_CONTENTS(num, obj);
          num->tag = NUM_INTPTR;
          num->data.v_intptr = LONGARG2;
          DEST = OBJ_VAL(obj);
          ++S.ip;
      }
        break;

      case OP_LOAD_UPVAL:
        DEST = UPVAL(ARG2);
        ++S.ip;
        break;

      case OP_LOAD_CELL:
        DEST = CELL(ARG2);
        ++S.ip;
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
         *  If OP is CALL_CELL or TAILCALL_CELL, ARG1 is an upval index. The
         *  upval does not refer to a function to call; rather it refers to a
         *  ref cell which refers to the function to call. Ideally, the upval
         *  should be statically guaranteed to be a ref cell, so tag-checking it
         *  should be unnecessary; for now, we do the check anyway. Checking
         *  that what the ref-cell points to is a function is, however,
         *  necessary.
         *
         *  If OP is CALL_REG or TAILCALL_REG, ARG1 is a register, which
         *  contains the function to be called. A type check is necessary as
         *  usual.
         *
         *  CALL_FUNC gets the closure being called for CALL and TAILCALL ops.
         *  CALL_REG_FUNC does the same for CALL_REG and TAILCALL_REG.
         */
#define CELL_FUNC CELL(ARG1)
#define REG_FUNC REG(ARG1)

        {
            val_t func;
            bool tail_call;

          case OP_CALL_CELL:
            func = CELL_FUNC;
            tail_call = false;

          call:
            /* We could avoid having do_call return a bool by just requiring
             * TAILCALL instrs to be followed by RETURN instrs; then do_call can
             * just increment IP if it wants to run the return instr. I'm not
             * sure whether this would gain us anything, though.
             */
            if (UNLIKELY(do_call(&S, tail_call, func, ARG2, ARG3)))
                goto op_return;
            break;

          case OP_CALL_REG:
            func = REG_FUNC;
            tail_call = false;
            goto call;

          case OP_TAILCALL_CELL:
            func = CELL_FUNC;
            tail_call = true;
            goto call;

          case OP_TAILCALL_REG:
            func = REG_FUNC;
            tail_call = true;
            goto call;
        }


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
        S.ip += (jump_offset_t) LONGARG2;
        break;

      op_return:
      case OP_RETURN: {
          /* Put the return value where it ought to be. */
          REG(0) = REG(ARG1);

          ++S.frame;              /* pop control stack (it grows down) */
          frame_tag_t frame_tag = *(frame_tag_t*) S.frame;
          switch ((enum frame_tag) EXPECT_LONG(frame_tag, FRAME_CALL)) {
            case FRAME_CALL: break;
              /* C calls inevitably come through our API, so the API function
               * that called us will do the necessary cleaning up. */
            case FRAME_C_CALL: return;
            default: IMPOSSIBLE("unrecognized or unimplemented frame tag: %u",
                                frame_tag);
          }
          /* Okay, we're returning into an Eris closure. */

          /* We determine how far to pop the stack by looking at the argument
           * offset given in the CALL instruction that set up our frame.
           */
          S.regs -= VM_ARG2(*S.frame->ip);
          S.ip = S.frame->ip + 1; /* +1 to skip past the call instr. */
          S.func = S.frame->func;
      }
        break;

      case OP_IF:
        do_cond(&S, !VAL_IS_NIL(REG(ARG1)));
        break;

      case OP_IFNOT:
        do_cond(&S, VAL_IS_NIL(REG(ARG1)));
        break;

        /* NB. logical CLOSE instr spans multiple (>= 2) instr_ts.
         * TODO: document format of CLOSE instr. */
      case OP_CLOSE: {
          upval_t nupvals_upvals = ARG2; /* # upvals from parent upvals */
          upval_t nupvals_regs = ARG3;   /* # upvals from registers */
          upval_t nupvals = nupvals_upvals + nupvals_regs;
          obj_t *funcobj = NEW_WITH(closure, upvals, nupvals);
          DEST = OBJ_VAL(funcobj);
          closure_t *func = OBJ_CONTENTS(closure, funcobj);

          /* TODO: carefully consider manually optimizing this. */

          /* Actually construct the closure. */
          ++S.ip;
          arg_t which_func = VM_ARG0(*S.ip);
          func->proto = S.func->proto->local_funcs[which_func];

          /* We read the indices from which to populate the new closure's upvals
           * from the instructions following the OP_CLOSE. */
          size_t i = 1;
          for (; i <= nupvals_upvals; ++i) {
              upval_t idx = VM_ARGN(*(S.ip + i / sizeof(instr_t)),
                                    i % sizeof(instr_t));
              func->upvals[i] = S.func->upvals[idx];
          }
          for (; i <= nupvals; ++i) {
              upval_t idx = VM_ARGN(*(S.ip + i / sizeof(instr_t)),
                                    i % sizeof(instr_t));
              func->upvals[i] = REG(idx);
          }
          S.ip += INTDIV_CEIL(1 + nupvals, sizeof(instr_t));
      }
        break;

      default:
        IMPOSSIBLE("unrecognized or unimplemented opcode: %u", OP);
    }

    goto begin;
}

/* Don't put anything here. */
