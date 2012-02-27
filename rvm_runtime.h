#ifndef _RVM_RUNTIME_H_
#define _RVM_RUNTIME_H_

#include <stdarg.h>

#include "rvm.h"

void rvm_vdie(const char *format, va_list ap);
void rvm_die(const char *format, ...);

/* These may need to be adjusted to take our state, to handle OOM exceptions. */

/* You pass the size of the object /sans/ tag. */
rvm_obj_t *rvm_new(rvm_tag_t tag, size_t objsize);
void rvm_free(rvm_obj_t *obj);


#endif
