#ifndef _RUNTIME_H_
#define _RUNTIME_H_

#include <stdarg.h>

#include "types.h"

void eris_vdie(const char *format, va_list ap);
void eris_die(const char *format, ...);

/* These will probably need adjusting. */
void eris_type_error(char *x, ...);
void eris_arity_error(char *x, ...);

/* These may need to be adjusted to take our state, to handle OOM exceptions. */

/* You pass the size of the object /sans/ tag. */
obj_t *eris_new(shape_t *tag, size_t objsize);
void eris_free(obj_t *obj);

#endif
