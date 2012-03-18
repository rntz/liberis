/* Not for public consumption. */
#ifndef _ERIS_MISC_H_
#define _ERIS_MISC_H_

#include <assert.h>

#define XCAT(X,Y) X##Y
#define CAT(X,Y) XCAT(X,Y)

#define STR_(x) #x
#define STR(x) STR_(x)

#define ARRAY_LEN(x) (sizeof(x)/sizeof((x)[0]))

/* Size of a member of a struct or union. */
#define membersize(type, mem) sizeof(((type*)NULL)->mem)

/* Clang defines __has_builtin, but other compilers don't. */
#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

/* This macro is used to inform the compiler that a particular point is
 * unreachable. Both clang and gcc use __builtin_unreachable for this purpose.
 *
 * FIXME: I have no idea which gcc version introduced __builtin_unreachable;
 * currently assuming they all have it.
 */
#if __has_builtin(__builtin_unreachable) || defined __GNUC__
#define UNREACHABLE (__builtin_unreachable())
#else
#define UNREACHABLE (assert(0 && "unreachable"))
#endif

/* These macros are used to inform the compiler that the boolean expression
 * contained within is "likely" (probably true) or "unlikely" (probably false),
 * respectively. Example usage:
 *
 *      if (LIKELY(expr)) {     // fast path or common case
 *          ...
 *      } else {                // slow path
 *          ...
 *      }
 *
 * FIXME: I have no idea which gcc version introduced __builtin_expect;
 * currently assuming they all have it.
 */
#if __has_builtin(__builtin_expect) || defined __GNUC__
#define LIKELY(x) (__builtin_expect(!!(x), 1))
#define UNLIKELY(x) (__builtin_expect(!!(x), 0))
#else
#define LIKELY(x) (!!(x))
#define UNLIKELY(x) (!!(x))
#endif

#endif
