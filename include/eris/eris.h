#ifndef _ERIS_ERIS_H_
#define _ERIS_ERIS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <eris/portability.h>

typedef struct eris_vm eris_vm_t;
typedef struct eris_thread eris_thread_t;
typedef struct eris_frame eris_frame_t;
typedef size_t eris_idx_t;
typedef intptr_t eris_int_t;
typedef uintptr_t eris_uint_t;
typedef double eris_float_t;

/* Creating vms, threads, and states. */
eris_vm_t *eris_vm_new(void);   /* can return NULL. */
void eris_vm_destroy(eris_vm_t *vm);

eris_thread_t *eris_thread_new(eris_vm_t *vm); /* can return NULL. */
void eris_thread_destroy(eris_thread_t *thread);

eris_frame_t *eris_frame_begin(eris_thread_t *thread);
void eris_frame_end(eris_frame_t *frame);

eris_thread_t *eris_frame_thread(eris_frame_t *frame);
eris_vm_t *eris_thread_vm(eris_thread_t *thread);


/* Miscellaneous stuff. */

/* A very primitive mechanism for dealing with eris exceptions. Calls
 * `try_func(state, data)'. Regardless of whether this caused exceptional
 * control flow, it then calls `finally_func(state, data)'. If the call to
 * `finally_func' causes exceptional control flow, it is propagated upward.
 * Otherwise, if the call to `try_func' caused exceptional control flow, that is
 * propagated outward. Otherwise, the call to eris_protect returns normally.
 */
void eris_protect(
    eris_frame_t *S,
    void (*try_func)(eris_frame_t*, void*),
    void (*finally_func)(eris_frame_t*, void*),
    void *data);


/* These functions use and manipulate the eris stack.
 * Indices are from the top of the stack. */
/* TODO: document */
void eris_pop(eris_frame_t *S, size_t num);
void eris_extend(eris_frame_t *S, size_t num);
void eris_move(eris_frame_t *S, eris_idx_t dst, eris_idx_t src);
void eris_copy(eris_frame_t *S, eris_idx_t dst, eris_idx_t src, size_t num);

/* Pushes the value in slot `idx'. */
void eris_dup(eris_frame_t *S, eris_idx_t idx);

/* Pops y, pops x; pushes (cons x y). Note the ordering of the pops. */
void eris_cons(eris_frame_t *S);

/* Args are on the top of the stack in reverse order. That is, the first arg is
 * at index nargs-1, the second at nargs-2, ..., the last at 0 (the top of the
 * stack). func_idx must be >= nargs.
 *
 * After the call, the stack has shrunk by (nargs-1) slots (if nargs is 0, it
 * has _grown_ by one slot), and the return value of the function is on top of
 * the stack (slot 0).
 */
void eris_call(eris_frame_t *S, eris_idx_t func_idx, size_t nargs);


/* Pushing data onto stack. */
void eris_push_int(eris_frame_t *S, eris_int_t i);
void eris_push_ratio(eris_frame_t *S, eris_int_t num, eris_uint_t denom);
void eris_push_float(eris_frame_t *S, eris_float_t f);

void eris_push_cons(eris_frame_t *S, eris_idx_t car_idx, eris_idx_t cdr_idx);
void eris_push_cstring(eris_frame_t *S, const char *string);
void eris_push_nil(eris_frame_t *S);
void eris_push_string(eris_frame_t *S, size_t len, const char *data);
void eris_push_symbol(eris_frame_t *S, size_t len, const char *data);

/* Pushes a closed-over value (an "upval"). */
void eris_push_upval(eris_frame_t *S, eris_idx_t upval_idx);

/* The type of a C function callback. Takes a frame and an arbitrary
 * user-supplied pointer. Returns the index on the stack of its return value.
 */
typedef eris_idx_t (*eris_c_func_t)(eris_frame_t*, void*);

/* Pushes a function with an empty closure (no upvals). */
void eris_push_func(eris_frame_t *S, eris_c_func_t func, void *data);

/* Pushes a function with a closure whose first `n_upval_idxs' upvals are taken
 * from the upvals of the current frame indicated by `upval_idxs', and whose
 * next `n_stack_idxs' upvals are taken from the stack values at indices
 * indicated by `stack_idxs'.
 *
 * Iff n_upval_idxs is 0, upval_idxs may be NULL. Likewise for {n_,}stack_idxs.
 */
void eris_push_closure(eris_frame_t *S, eris_c_func_t func,
                       size_t n_upval_idxs, eris_idx_t *upval_idxs,
                       size_t n_stack_idxs, eris_idx_t *stack_idxs,
                       void *data);


/* Getting data from stack. */
bool eris_is_cons(eris_frame_t *S, eris_idx_t idx);
bool eris_is_num(eris_frame_t *S, eris_idx_t idx);
bool eris_is_nil(eris_frame_t *S, eris_idx_t idx);
bool eris_is_string(eris_frame_t *S, eris_idx_t idx);
bool eris_is_symbol(eris_frame_t *S, eris_idx_t idx);

ERIS_WARN_UNUSED_RESULT
bool eris_check_string_len(eris_frame_t *S, eris_idx_t idx, size_t *lenp);

ERIS_WARN_UNUSED_RESULT
bool eris_check_symbol_len(eris_frame_t *S, eris_idx_t idx, size_t *lenp);

/* Returns "false" if either the value at `idx' isn't a number or if it can't be
 * represented in an eris_int_t.
 */
ERIS_WARN_UNUSED_RESULT
bool eris_check_int(eris_frame_t *S, eris_idx_t idx, eris_int_t *out);

/* TODO: floating-point, rational, and arbitrary-precision number getters. */

/* The "eris_get_*" functions cause exceptional control flow if index accessed
 * does not have an object of the appropriate type in it.
 */
/* Returns the size of the string. If this is > `len' then the string was
 * truncated to fit in `buf'. DOES NOT NULL-TERMINATE `buf'. Suggest using
 * eris_check_string_len to determine how large to make `buf'.
 */
size_t eris_get_string(eris_frame_t *s, eris_idx_t idx, size_t len, char *buf);

/* Always null-terminates `buf'. Assumes `buf' is of size precisely `len'.
 * Because of this, if return value is >= `len' ("greater than or equal to",
 * unlike eris_get_string's "greater than"), then the string was truncated.
 */
size_t eris_get_cstring(eris_frame_t *s, eris_idx_t idx, size_t len, char *buf);


/* Loading compiled chunks. Treat all these functions as if they might cause
 * exceptional control flow. (Most of them can. Some can't, but that may change
 * in the future.)
 */
void eris_loader_open_file(eris_frame_t *S, const char *filename);
void eris_loader_open_buf(eris_frame_t *S, size_t len, const char *buf);
void eris_loader_open_string(eris_frame_t *S);
/* Calling eris_loader_open_FILE relinquishes the caller's ownership of `file'.
 * It is Eris' now. */
void eris_loader_open_FILE(eris_frame_t *S, FILE *file);

/* Pushes what it loads on top of the stack. At present, the only things that
 * can be loaded are functions.
 */
void eris_loader_load(eris_frame_t *S, eris_idx_t idx);

/* In some future version of Eris, the garbage collector will ensure that
 * unreferenced loaders eventually get closed. Until then it is your duty to
 * close every loader you open. It's probably a good idea to do so anyway.
 */
void eris_loader_close(eris_frame_t *S, eris_idx_t idx);

#endif
