#ifndef VM_SWAP_H
#define VM_SWAP_H

#include <stddef.h>
#include <stdbool.h>
#include <inttypes.h>

void swap_init(void);
size_t swap_out(void *);
bool swap_in(size_t, void *);
void swap_free(size_t);

#endif
