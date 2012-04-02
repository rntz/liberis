#ifndef _API_H_
#define _API_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* We use Judy arrays for various things. Not because we've actually determined
 * that we need the speed and memory characteristics of Judy arrays, just
 * because they present a nice, simple interface.
 */
#include <Judy.h>

#include <eris/eris.h>

#include "types.h"
#include "vm.h"

struct eris_vm {
    /* TODO: GC metadata. */
    /* Judy array of interned symbols. */
    Pvoid_t symbols;
    /* Linked list of threads. */
    eris_thread_t *threads;
};

struct eris_thread {
    eris_vm_t *vm;
    /* Whether a frame has been created on this thread and is in use. */
    bool in_use;
    /* `regs' points to bottom of register stack. */
    val_t *regs;
    /* `frames' points to top of frame stack. dereferencing it is disallowed. */
    void *frames;
    /* TODO: GC metadata (eg. size of register & frame stacks). */
    /* Next thread on thread list. */
    eris_thread_t *next;
};

struct eris_frame {
    size_t num_regs;
    /* Points to the bottom of our chunk of the register stack. */
    val_t *regs;
    /* Points to our frame on the control stack, which is just below the frame
     * we return into. */
    void *frames;
    eris_thread_t *thread;
};

#endif
