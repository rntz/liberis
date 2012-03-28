/* Don't include this file. Include misc.h instead. */
#ifndef _PORTABILITY_H_
#define _PORTABILITY_H_

#include <stdbool.h>

#include <eris/portability.h>

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
#define EXPECT_LONG __builtin_expect
#else
#define EXPECT_LONG(x, v) ((v), (x))
#endif

#define EXPECT_BOOL(x, v) ((bool) EXPECT_LONG(!!(x), (long)(v)))
#define LIKELY(x) EXPECT_BOOL(x, true)
#define UNLIKELY(x) EXPECT_BOOL(x, false)

/* FIXME: given gcc or clang, assumes sufficiently recent */
#if defined __GNUC__ || defined __clang__
#define HAVE_PRAGMA_GCC_DIAGNOSTIC 1
#endif

#endif
