#ifndef _RUNTIME_H_
#define _RUNTIME_H_

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>

#include "portability.h"
#include "types.h"


/* Core runtime functions */

/* Interface to the VM loop. */
void eris_vm_run(vm_state_t *state);

/* Sets `*out` to the newly allocated object on success.
 * `size` should be the size of the object /including/ the tag.
 *
 * Returns false iff allocation failed due to OOM; caller should raise an
 * exception in this case.
 */
ERIS_WARN_UNUSED_RESULT
bool eris_new(obj_t **out,
              eris_thread_t *thread, frame_t *frame,
              shape_t *tag, size_t size);

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

/* Error-raising functions */
NORETURN void eris_vbug(const char *format, va_list ap);
NORETURN void eris_bug(const char *format, ...);

#if ERIS_RELEASE
#define IMPOSSIBLE(msg, ...) UNREACHABLE
#else
#define IMPOSSIBLE(msg, ...) (eris_bug(msg, __VA_ARGS__))
#endif


#endif
