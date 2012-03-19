#ifndef _ERIS_H_
#define _ERIS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include <eris_portability.h>

typedef struct eris_vm eris_vm_t;
typedef struct eris_state eris_state_t;
typedef size_t eris_idx_t;
typedef uint32_t eris_int_t;

/* Creating vms, threads, and states. */
eris_vm_t *eris_vm_new(void);   /* can return NULL. */
void eris_vm_destroy(eris_vm_t *vm);

eris_state_t *eris_state_new(eris_vm_t *vm); /* can return NULL. */
void eris_state_destroy(eris_state_t *state);

/* TODO: builtin functions. */


/* Miscellaneous stuff. */

/* These functions return true if `v' was successfully encoded into `out'. They
 * may return false because Eris may not be able to represent all values of the
 * appropriate C types.
 *
 * In fact, Eris presently can only represent 31-bit integers; that is, unsigned
 * values in the range 0..2^31-1 and signed values in the range -2^30..2^30-1.
 * This restriction is unlikely to change anytime soon.
 */
bool eris_uint_encode(uint32_t val, eris_int_t *out);
bool eris_int_encode ( int32_t val, eris_int_t *out);
uint32_t eris_uint_decode(eris_int_t val);
 int32_t eris_int_decode (eris_int_t val);

/* A very primitive mechanism for dealing with eris exceptions. Calls
 * `try_func(state, data)'. Regardless of whether this caused exceptional
 * control flow, it then calls `finally_func(state, data)'. If the call to
 * `finally_func' causes exceptional control flow, it is propagated upward.
 * Otherwise, if the call to `try_func' caused exceptional control flow, that is
 * propagated outward. Otherwise, the call to eris_protect returns normally.
 */
void eris_protect(
    eris_state_t *S,
    void (*try_func)(eris_state_t*, void*),
    void (*finally_func)(eris_state_t*, void*),
    void *data);


/* These functions use and manipulate the eris stack.
 * Indices are from the top of the stack. */
/* TODO: document */
void eris_pop(eris_state_t *S, size_t num);
void eris_extend(eris_state_t *S, size_t num);
void eris_move(eris_state_t *S, eris_idx_t dst, eris_idx_t src);
void eris_copy(eris_state_t *S, eris_idx_t dst, eris_idx_t src, size_t num);

/* Pushes the value in slot `idx'. */
void eris_dup(eris_state_t *S, eris_idx_t idx);

/* Pops y, pops x; pushes (cons x y). Note the ordering of the pops. */
void eris_cons(eris_state_t *S);

/* Args are on the top of the stack in reverse order. That is, the first arg is
 * at index nargs-1, the second at nargs-2, ..., the last at 0 (the top of the
 * stack). func_idx must be >= nargs.
 *
 * After the call, the stack has shrunk by (nargs-1) slots (if nargs is 0, it
 * has _grown_ by one slot), and the return value of the function is on top of
 * the stack (slot 0).
 */
void eris_call(eris_state_t *S, eris_idx_t func_idx, size_t nargs);

/* Pushing data onto stack. */
/* TODO: how do I create C closures referencing eris values? */
void eris_push_func(eris_state_t *S,
                    /* returns the index of the return value. */
                    eris_idx_t (*func)(eris_state_t*, void*),
                    void *data);

void eris_push_cons(eris_state_t *S, eris_idx_t car_idx, eris_idx_t cdr_idx);
void eris_push_cstring(eris_state_t *S, const char *string);
void eris_push_nil(eris_state_t *S);
void eris_push_string(eris_state_t *S, size_t len, const char *data);
void eris_push_symbol(eris_state_t *S, size_t len, const char *data);

/* Getting data from stack. */
bool eris_is_cons(eris_state_t *S, eris_idx_t idx);
bool eris_is_int(eris_state_t *S, eris_idx_t idx);
bool eris_is_nil(eris_state_t *S, eris_idx_t idx);
bool eris_is_string(eris_state_t *S, eris_idx_t idx);
bool eris_is_symbol(eris_state_t *S, eris_idx_t idx);

ERIS_WARN_UNUSED_RESULT
bool eris_check_string_len(eris_state_t *S, eris_idx_t idx, size_t *lenp);

ERIS_WARN_UNUSED_RESULT
bool eris_check_symbol_len(eris_state_t *S, eris_idx_t idx, size_t *lenp);

ERIS_WARN_UNUSED_RESULT
bool eris_check_int(eris_state_t *S, eris_idx_t idx, eris_int_t *valp);

/* The "eris_get_*" functions cause exceptional control flow if index accessed
 * does not have an object of the appropriate type in it.
 */
/* Returns the size of the string. If this is > `len' then the string was
 * truncated to fit in `buf'. DOES NOT NULL-TERMINATE `buf'. Suggest using
 * eris_check_string_len to determine how large to make `buf'.
 */
ERIS_WARN_UNUSED_RESULT
size_t eris_get_string(
    eris_state_t *s, eris_idx_t idx, size_t len, char *buf);


/* Loading compiled chunks. */
void eris_loader_open_file(eris_state_t *S, const char *filename);
void eris_loader_open_buf(eris_state_t *S, size_t len, const char *buf);
void eris_loader_open_string(eris_state_t *S);
/* Calling eris_loader_open_FILE relinquishes the caller's ownership of `file'.
 * It is Eris' now. */
void eris_loader_open_FILE(eris_state_t *S, FILE *file);

/* Pushes what it loads on top of the stack. At present, the only things that
 * can be loaded are functions.
 */
void eris_loader_load(eris_state_t *S, eris_idx_t idx);

/* In some future version of Eris, the garbage collector will ensure that
 * unreferenced loaders eventually get closed. Until then it is your duty to
 * close every loader you open. It's probably a good idea to do so anyway.
 */
void eris_loader_close(eris_state_t *S, eris_idx_t idx);

#endif
