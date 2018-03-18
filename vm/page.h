#ifndef VM_PAGE_H
#define VM_PAGE_H

#include <stddef.h>
#include <inttypes.h>
#include <hash.h>

struct page {
	struct hash_elem elem;
	void *upage;
	bool valid; // Is this page valid?
	bool swap; // Is this page swapped out or not?
	size_t idx; // If page is in memory, idx is frame number. Otherwise, idx is swap number
};

void page_init(struct hash *);
void page_valid(struct hash *spt, void *upage, bool swap, size_t idx);
void page_invalid(struct hash *spt, void *upage);
struct page* page_get(struct hash *spt, void *upage);
void page_free(struct hash *spt, void *upage);
void page_destroy(struct hash *spt);

#endif
