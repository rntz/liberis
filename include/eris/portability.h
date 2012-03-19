/* Compiler-dependent macros. */
#ifndef _ERIS_PORTABILITY_H_
#define _ERIS_PORTABILITY_H_

#ifndef __has_attribute
#define ERIS_HAS_ATTR_DEFINED
#define __has_attribute(x) 0
#endif

/* FIXME: given gcc, assumes sufficiently recent gcc */
#if defined __GNUC__ || __has_attribute(__warn_unused_result__)
#define ERIS_WARN_UNUSED_RESULT __attribute__((__warn_unused_result__))
#else
#define ERIS_WARN_UNUSED_RESULT
#endif

#ifdef ERIS_HAS_ATTR_DEFINED
#undef __has_attribute
#undef ERIS_HAS_ATTR_DEFINED
#endif

#endif
