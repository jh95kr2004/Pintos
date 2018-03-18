#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <hash.h>
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "vm/swap.h"
#include "vm/frame.h"
#include "vm/page.h"

static unsigned page_hash_hash_func(const struct hash_elem *e, void *aux) {
	return hash_int((int)hash_entry(e, struct page, elem)->upage);
}

static bool page_hash_less_func(const struct hash_elem *a, const struct hash_elem *b, void *aux) {
	return (hash_entry(a, struct page, elem)->upage < hash_entry(b, struct page, elem)->upage);
}

static void page_hash_clean_func(struct hash_elem *e, void *aux) {
	struct page *page = hash_entry(e, struct page, elem);
	if(page->valid == true) {
		if(!page->swap) frame_free(page->idx);
		else swap_free(page->idx);
	}
}

static void page_hash_destroy_func(struct hash_elem *e, void *aux) {
	struct page *page = hash_entry(e, struct page, elem);
	free(page);
}

/* Initialize supplemental page table */
void page_init(struct hash *spt) {
	hash_init(spt, page_hash_hash_func, page_hash_less_func, NULL);
}

static struct page* page_find(struct hash *spt, void *upage, bool create) {
	//printf("(find) spt : %p / page_num : %d", spt, pg_no(upage));
    struct page p, *page = NULL;
	struct hash_elem *he;
	p.upage = upage;
	he = hash_find(spt, &p.elem);
    if(he != NULL) page = hash_entry(he, struct page, elem);
    else if(create) {
        page = malloc(sizeof(struct page));
        if(page != NULL) {
            page->upage = upage;
            page->valid = false;
            hash_insert(spt, &page->elem);
        }
    }
    return page;
}

/* If a page is move into the frame or swap space, then set the information of page.
   If SWAP is false, the page is in the frame, otherwise is in the swap space.
   IDX is a index value, where is page located. */
void page_valid(struct hash *spt, void *upage, bool swap, size_t idx) {
	//printf("(valid) upage : %p / page_num : %d / swap : %d / idx : %d\n", upage, pg_no(upage), swap, idx);
	struct page *page = page_find(spt, upage, true);
    if(page == NULL) return;
	page->valid = true;
	page->swap = swap;
	page->idx = idx;
}

void page_invalid(struct hash *spt, void *upage) {
	//printf("(invalid) spt : %p / page_num : %d\n", spt, pg_no(upage));
    struct page *page = page_find(spt, upage, true);
    if(page == NULL) return;
	page->valid = false;
}

/* Find the page in the supplemental page table */
struct page* page_get(struct hash *spt, void *upage) {
	//printf("(get) spt : %p / page_num : %d\n", spt, pg_no(upage));
	return page_find(spt, upage, false);
}

/* Free page entry of supplemental page table. */
void page_free(struct hash *spt, void *upage) {
	struct page *page = page_find(spt, upage, false);
	hash_delete(spt, &page->elem);
	if(page != NULL) free(page);
}

/* This function should be called when the thread goes to die. */
void page_destroy(struct hash *spt) {
	hash_apply(spt, page_hash_clean_func);
	hash_destroy(spt, page_hash_destroy_func);
}
