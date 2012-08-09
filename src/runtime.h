#ifndef _RUNTIME_H_
#define _RUNTIME_H_

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* We use Judy arrays for various things. Not because we've actually determined
 * that we need the speed and memory characteristics of Judy arrays, just
 * because they present a nice, simple interface.
 */
#include <Judy.h>

#include <eris/eris.h>

#include "misc.h"
#include "portability.h"
#include "types.h"
#include "vm.h"

/* Runtime data structures */
struct eris_vm {
    /* TODO: GC metadata. */
    /* Judy array of interned symbols. */
    Pvoid_t symbols;
    val_t symbol_t;         /* the "t" symbol, used as a canonical true value */
    /* Linked list of threads. */
    eris_thread_t *threads;
};

struct eris_thread {
    eris_vm_t *vm;
    /* Whether a frame has been created on this thread and is in use. */
    bool in_use;
    /* `regs' points to bottom of register stack. */
    val_t *regs;
    /* `frames' points to top of frame stack. dereferencing it is disallowed. */
    void *frames;
    /* TODO: GC metadata (eg. size of register & frame stacks). */
    /* Next thread on thread list. */
    eris_thread_t *next;
};

struct eris_frame {
    /* Points to the bottom of our chunk of the register stack. */
    val_t *regs;
    /* How many registers are we using? Needed for push/pop/etc and for GC. */
    size_t num_regs;
    /* Points to our frame on the control stack, which is just below the frame
     * we return into. */
    frame_t *frame;
    eris_thread_t *thread;
    /* FIXME: need tailcall info */
};


/* Core runtime functions
 *
 * Many of these functions take as their first arguments:
 *
 *     eris_thread_t *T, void *F, ...
 *
 * `F` points to the current (ie. bottom) frame on the control stack. Together,
 * T & F provide the information necessary to perform a GC cycle or unwind the
 * stack due to an exception.
 */

/* You pass the size of the object /sans/ tag. */
obj_t *eris_new(eris_thread_t *T, void *F, shape_t *tag, size_t size);
void eris_free(eris_thread_t *T, void *F, obj_t *obj);

/* Convenience functions. */
static inline
val_t eris_make_true(eris_vm_t *vm) { return vm->symbol_t; }

static inline
val_t eris_make_false(eris_vm_t *vm) { return eris_nil; (void) vm; }

static inline
val_t eris_make_bool(eris_vm_t *vm, bool v)
{
    return v ? eris_make_true(vm) : eris_make_false(vm);
}

/* These will probably need adjusting. */
NORETURN void eris_type_error(eris_thread_t *T, void *F, char *fmt, ...);
NORETURN void eris_arity_error(eris_thread_t *T, void *F, char *fmt, ...);

/* Bug checking. */
NORETURN void eris_bug(const char *format, ...);
NORETURN void eris_vbug(const char *format, va_list ap);

#if ERIS_RELEASE
#define IMPOSSIBLE(msg, ...) UNREACHABLE
#else
#define IMPOSSIBLE(msg, ...) (eris_bug(msg, __VA_ARGS__))
#endif

#endif
