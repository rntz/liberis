/* Don't include this file. Include misc.h instead. */
#ifndef _PORTABILITY_H_
#define _PORTABILITY_H_

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>             /* for abort() */

#include <eris/portability.h>


/* ---------- Compiler-dependent macros ----------
 *
 * Macros defined in a compiler-dependent fashion by this file, and their
 * purposes (in parentheses):
 *
 * - NORETURN: Marks a function as not returning. (optimization, warnings)
 *
 * - UNREACHABLE: expression that never executes; indicates unreachability.
 *   (optimization)
 *
 * - EXPECT_LONG(x, v): Indicates that we "expect" x to have the value v, which
 *   should be a constant. (optimization)
 *
 *   Avoid direct use of this macro; prefer LIKELY() and UNLIKELY(), defined
 *   further down in this file.
 *
 * Define IGNORE_COMPILER_FEATURES to force all of these to use their default,
 * standards-compliant, non-compiler-specific definitions.
 */

#ifndef IGNORE_COMPILER_FEATURES
#ifdef __GNUC__

/* FIXME: should be more cautious about what features gcc supports, based on
 * version numbers */
#define NORETURN __attribute__((__noreturn__))
#define UNREACHABLE (__builtin_unreachable())
#define EXPECT_LONG __builtin_expect

#else  /* __GNUC__ */

#ifdef __has_attribute
#if __has_attribute(__noreturn__)
#define NORETURN __attribute__((__noreturn__))
#endif
#endif  /* __has_attribute */

#ifdef __has_builtin
#if __has_builtin(__builtin_unreachabe)
#define UNREACHABLE (__builtin_unreachable())
#endif
#if __has_builtin(__builtin_expect)
#define EXPECT_LONG __builtin_expect
#endif
#endif  /* __has_builtin */

#endif  /* __GNUC__ */
#endif  /* IGNORE_COMPILER_FEATURES */

/* Portable definitions. */
#ifndef NORETURN
#define NORETURN
#endif

#ifndef UNREACHABLE
/* the abort() is necessary to ensure this does not return when NDEBUG is
 * defined. */
#define UNREACHABLE (assert(0 && "unreachable"), abort())
#endif

#ifndef EXPECT_LONG
#define EXPECT_LONG(x,v) ((void)(v),(x))
#endif


/* ---------- Derived macros ----------
 *
 * These macros are defined entirely in terms of the above macros, so they will
 * automatically take advantage of compiler-specific features iff available.
 */
#define EXPECT_BOOL(x,v) ((bool) EXPECT_LONG(!!(x), (long)(v)))
#define LIKELY(x) EXPECT_BOOL(x, true)
#define UNLIKELY(x) EXPECT_BOOL(x, false)

#endif
