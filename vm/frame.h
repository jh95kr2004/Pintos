#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <stddef.h>
#include <inttypes.h>

void frame_init(void);
void* frame_alloc(void *upage, enum palloc_flags flags, bool writable);
void frame_free(uint32_t frame_num);

#endif
