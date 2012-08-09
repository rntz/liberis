#include <assert.h>
#include <string.h>

#include <eris/eris.h>

#include "misc.h"
#include "runtime.h"
#include "types.h"
#include "vm.h"

#define FRAME(f) (f)->data.eris_call

/* Helper functions. */
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
        S->ip += 1 + VM_SIGNED_LONGARG(*(S->ip + 1));
}


/* The main loop */

/* TODO: Use computed gotos if available. See femtolisp's flisp.c for a good way
 * to do this.
 */

void eris_vm_run(vm_state_t *state)
{
    vm_state_t S = *state;
#define REG(n)      (S.regs[n])

    if (0) {
      raise:
        /* TODO: Stack-unwinding code */
        assert (0 && "unimplemented");
    }

#define NEW(shape, ...) do {                                    \
        if (!new_##shape(__VA_ARGS__, S.thread, S.frame)) {     \
            goto raise;                                         \
        }                                                       \
    } while (0)

#define NEW_SEQ(...) NEW(seq, __VA_ARGS__)
#define NEW_NUM(...) NEW(num, __VA_ARGS__)
#define NEW_CLOSURE(...) NEW(closure, __VA_ARGS__)

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
#define LONGARG VM_LONGARG(instr)
#define SIGNED_LONGARG VM_SIGNED_LONGARG(instr)

#define UPVAL(upval) (S.func->upvals[(upval)])
#define CELL(upval) (deref_cell(get_cell(UPVAL(upval))))

    /* TODO: order cases by frequency. */
    switch ((enum op) OP) {
      case OP_MOVE:
        REG(ARG1) = REG(ARG2);
        ++S.ip;
        break;

      case OP_LOAD_INT: {
          /* Allocating; need to update frame IP in case of GC scan. */
          FRAME(S.frame).ip = S.ip;

          num_t *num;
          NEW_NUM(&num);
          num->tag = NUM_INTPTR;
          num->data.v_intptr = SIGNED_LONGARG;
          REG(ARG1) = CONTENTS_VAL(num);
          ++S.ip;
      }
        break;

      case OP_LOAD_UPVAL:
        REG(ARG1) = UPVAL(ARG2);
        ++S.ip;
        break;

      case OP_LOAD_CELL:
        REG(ARG1) = CELL(ARG2);
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
            val_t funcval;
            bool tail_call;

          case OP_CALL_CELL:
            funcval = CELL_FUNC;
            tail_call = false;
            goto call;

          case OP_CALL_REG:
            funcval = REG_FUNC;
            tail_call = false;
            goto call;

          case OP_TAILCALL_CELL:
            funcval = CELL_FUNC;
            tail_call = true;
            goto call;

          case OP_TAILCALL_REG:
            funcval = REG_FUNC;
            tail_call = true;
            goto call;

          call: (void) 0;
            obj_t *funcobj = VAL_OBJ(funcval);
            reg_t offset = ARG2;
            nargs_t nargs = ARG3;

            /* Calling closures */
            if (LIKELY(funcobj->tag == SHAPE_TAG(closure))) {
                closure_t *func = OBJ_CONTENTS(closure, funcobj);

                /* TODO: think very hard about what happens on control stack
                 * overflow. */

                /* Check arity. */
                if (UNLIKELY(nargs != func->proto->num_args)) {
                    if (LIKELY(func->proto->variadic)) {
                        /* TODO */
                        eris_bug("variadic function calls unimplemented");
                    }
                    else {
                        goto raise; /* TODO: arity error */
                    }
                }

                if (!tail_call) {
                    /* Update our IP on control stack, so callee returns
                     * correctly. */
                    FRAME(S.frame).ip = S.ip;

                    /* Push callee's stack frame. Remember, control stack grows
                     * down. */
                    --S.frame;
                    S.frame->tag = FRAME_CALL;
                    /* No need to set frame's IP; callee handles that.
                     * frame's func gets set unconditionally, below. */

                    /* Shift our view of the register stack so our args are in
                     * the right place. */
                    S.regs += offset;
                }
                else {     /* tail_call is true */
                    /* No need to update frame's IP; callee handles that.
                     * No need to update frame's tag; already FRAME_CALL.
                     * frame's func gets set unconditionally, below.
                     */

                    /* Move down arguments into appropriate slots. */
                    memmove(S.regs, S.regs + offset, sizeof(val_t) * nargs);
                }

                /* Update frame. */
                FRAME(S.frame).func = func;

                /* Jump into the function. */
                S.func = func;
                S.ip = S.func->proto->code;
            }
            /* Calling builtins */
            else if (LIKELY(funcobj->tag == SHAPE_TAG(builtin))) {
                builtin_t *builtin = OBJ_CONTENTS(builtin, funcobj);
                if (UNLIKELY(nargs != builtin->num_args)
                    && (UNLIKELY(!builtin->variadic)
                        || UNLIKELY(nargs < builtin->num_args)))
                {
                    goto raise; /* TODO: arity error */
                }

                switch (builtin->op) {
#define BUILTIN(name, num_args, variadic, ...)                  \
                    case CAT(BOP_,name): { __VA_ARGS__ } break;
#define ARG(i)  S.regs[offset+(i)]
#define DEST    S.regs[offset]
#define UNIMPLEMENTED eris_bug("builtin %u unimplemented", builtin->op);

#include "builtins.expando"

#undef UNIMPLEMENTED
#undef DEST
#undef ARG
#undef BUILTIN

                  default: IMPOSSIBLE("unrecognized builtin: %u", builtin->op);
                }

                /* Incrementing IP works even if tail_call is true, since then
                 * next instr is guaranteed to be an OP_RETURN. */
                ++S.ip;
            }
            /* Calling C closures */
            else if (LIKELY(funcobj->tag == SHAPE_TAG(c_closure))) {
                assert(0 && "unimplemented"); /* TODO */
                /* TODO: what if the C func calls eris_c_tailcall? */
                /* Incrementing IP works even if tail_call is true, since then
                 * next instr is guaranteed to be an OP_RETURN */
                ++S.ip;
            }
            /* Calling everything else */
            else {
                goto raise; /* TODO: type error */
            }

            break;
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
        S.ip += SIGNED_LONGARG;
        break;

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
          S.regs -= VM_ARG2(*FRAME(S.frame).ip);
          S.ip = FRAME(S.frame).ip + 1; /* +1 to skip past the call instr. */
          S.func = FRAME(S.frame).func;
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
          /* Allocating; need to update frame IP in case of GC scan. */
          FRAME(S.frame).ip = S.ip;

          upval_t nupvals_upvals = ARG2; /* # upvals from parent upvals */
          upval_t nupvals_regs = ARG3;   /* # upvals from registers */
          upval_t nupvals = nupvals_upvals + nupvals_regs;
          closure_t *func;
          NEW_CLOSURE(&func, nupvals);
          REG(ARG1) = CONTENTS_VAL(func);

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
