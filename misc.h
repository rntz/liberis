/* Not for public consumption. */

#ifndef _RVM_MISC_H_
#define _RVM_MISC_H_

#define XCAT(X,Y) X##Y
#define CAT(X,Y) XCAT(X,Y)

/* Size of a member of a struct or union. */
#define membersize(type, mem) sizeof(((type*)NULL)->mem)

/* Clang defines __has_builtin, but other compilers don't. */
#ifndef __has_builtin
#define __has_builtin(x) 0
#endif

/* This macro is used to inform the compiler that a particular point is
 * unreachable. Both clang and gcc use __builtin_unreachable for this purpose.
 *
 * I have no idea which gcc version introduced __builtin_unreachable, so I'm
 * just gonna assume they all have it.
 */
#if __has_builtin(__builtin_unreachable) || defined __GNUC__
#define UNREACHABLE (__builtin_unreachable())
#else
#define UNREACHABLE (assert(0 && "unreachable"))
#endif

/* TODO: document */
#if __has_builtin(__builtin_expect) || defined __GNUC__
#define LIKELY(x) (__builtin_expect(!!(x), 1))
#define UNLIKELY(x) (__builtin_expect(!!(x), 0))
#else
#define LIKELY(x) (!!(x))
#define UNLIKELY(x) (!!(x))
#endif

#endif
