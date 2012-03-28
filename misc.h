#ifndef _MISC_H_
#define _MISC_H_

#include <assert.h>

#include "portability.h"

#define XCAT(X,Y) X##Y
#define CAT(X,Y) XCAT(X,Y)

#define STR_(x) #x
#define STR(x) STR_(x)

#define ARRAY_LEN(x) (sizeof(x)/sizeof((x)[0]))

/* Size of a member of a struct or union. */
#define membersize(type, mem) sizeof(((type*)NULL)->mem)

#endif
