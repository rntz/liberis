/* This is an expando-style header. It is meant to be included with the BUILTIN
 * defined to something that will do a useful thing for each builtin definition.
 * As such, it has no include guards.
 *
 * BUILTIN(name, ...)
 *
 * - name: the name of the builtin. all caps, suitable for use in a macro name.
 *
 * - __VA_ARGS__: code that evaluates the builtin. expects the following macros
 *   to be defined:
 *
 *   - NARGS: number of arguments
 *   - ARG(i): ith argument
 *   - DEST: location to put the result value in.
 *   - ARITY_ERROR(): signals an arity error.
 *   - TYPE_ERROR(): signals a type error.
 *   - UNIMPLEMENTED: signals the builtin is unimplemented.
 *
 *   TODO: TYPE_ERROR (and maybe ARITY_ERROR?) should take args to indicate what
 *   failed.
 *
 *   TODO: list headers we expect to be included for code to work.
 */

#ifndef BUILTIN
#error "builtins.h included with BUILTIN undefined"
#endif

/* Conses */
BUILTIN(CONS, UNIMPLEMENTED)
BUILTIN(CAR, UNIMPLEMENTED)
BUILTIN(CDR, UNIMPLEMENTED)

/* Strings */
BUILTIN(STR_CAT, UNIMPLEMENTED) /* variadic */
BUILTIN(STR_EQ, UNIMPLEMENTED)
BUILTIN(STR_CMP, UNIMPLEMENTED)
BUILTIN(STR_SUBSTR, UNIMPLEMENTED)
BUILTIN(INTERN, UNIMPLEMENTED)

/* Equality, comparison, other tests */
/* Should equality tests be variadic? */
/* fastest & crudest equality test */
BUILTIN(RAW_EQ, UNIMPLEMENTED)
BUILTIN(NUM_EQ, UNIMPLEMENTED)
BUILTIN(SYM_EQ, UNIMPLEMENTED)
BUILTIN(IS_NIL, UNIMPLEMENTED)

/* Miscellany. */
BUILTIN(APPLY, UNIMPLEMENTED)   /* variadic */


/* Arithmetic */
/* add, sub, mul, div all variadic */
BUILTIN(ADD, UNIMPLEMENTED)
BUILTIN(SUB, UNIMPLEMENTED)     /* with 1 arg, negates */
BUILTIN(MUL, UNIMPLEMENTED)
BUILTIN(DIV, UNIMPLEMENTED)     /* with 1 arg, inverts */

BUILTIN(MOD, UNIMPLEMENTED)
BUILTIN(MIN, UNIMPLEMENTED)     /* variadic, >0 args */
BUILTIN(MAX, UNIMPLEMENTED)     /* variadic, >0 args */
BUILTIN(ABS, UNIMPLEMENTED)

BUILTIN(SQRT, UNIMPLEMENTED)
BUILTIN(EXP, UNIMPLEMENTED)     /* 2adic (b, n) --> b^n
                                 * or 1adic (n) --> e^n */
BUILTIN(LOG, UNIMPLEMENTED)     /* 2adic (b, n) --> log_b(n)
                                 * or 1adic (n) --> log_e(n) */

/* I do NOT (yet?) guarantee these behave as in r5rs.
 * TODO: think about semantics. */
BUILTIN(EXACT_TO_INEXACT, UNIMPLEMENTED)
BUILTIN(INEXACT_TO_EXACT, UNIMPLEMENTED)

/* These SHOULD behave as in r5rs. */
BUILTIN(FLOOR, UNIMPLEMENTED)
BUILTIN(CEILING, UNIMPLEMENTED)
BUILTIN(TRUNCATE, UNIMPLEMENTED)
BUILTIN(ROUND, UNIMPLEMENTED)

/* Bitwise ops. Produce errors on input that isn't integral. */
BUILTIN(BIT_AND, UNIMPLEMENTED)
BUILTIN(BIT_OR, UNIMPLEMENTED)
BUILTIN(BIT_XOR, UNIMPLEMENTED)
BUILTIN(BIT_NOT, UNIMPLEMENTED)
/* Right-shifts are always arithmetic, not logical, since we simulate
 * unlimited-precision arithmetic.
 */
BUILTIN(BIT_SHR, UNIMPLEMENTED)
BUILTIN(BIT_SHL, UNIMPLEMENTED)
