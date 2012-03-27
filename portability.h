#ifndef _PORTABILITY_H_
#define _PORTABILITY_H_

#include <eris/portability.h>

/* FIXME: given gcc or clang, assumes sufficiently recent */
#if defined __GNUC__ || defined __clang__
#define HAVE_PRAGMA_GCC_DIAGNOSTIC 1
#endif

#endif
