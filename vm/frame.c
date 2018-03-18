#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <bitmap.h>
#include <list.h>
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "threads/pte.h"
#include "userprog/pagedir.h"
#include "vm/frame.h"
#include "vm/swap.h"
#include "vm/page.h"

struct list frame_list;
struct frame_elem {
	struct list_elem elem;
	uint32_t frame_num;
    void *pd;
	void *spt;
	void *upage;
	void *kpage;
};
struct lock frame_lock;
struct frame_elem *victim;

/* Initialize frame table and frame bitmap */
void frame_init(void) {
	list_init(&frame_list);
	lock_init(&frame_lock);
    victim = NULL;
}

static struct frame_elem* get_frame(uint32_t frame_num, bool create) {
    struct frame_elem *e, *find = NULL;
    struct list_elem *le;
    for(le = list_begin(&frame_list); le != list_end(&frame_list); le = list_next(le)) {
        e = list_entry(le, struct frame_elem, elem);
        if(e->frame_num == frame_num) {
            find = e;
            break;
        }
    }
    if(find == NULL && create) {
        find = malloc(sizeof(struct frame_elem));
        if(find != NULL) {
            if(victim == NULL) {
                list_insert (list_tail(&frame_list), &find->elem);
                victim = find;
            }
            else list_insert (&victim->elem, &find->elem);
        }
    }
    return find;
}

static void free_frame(struct frame_elem *frame) {
    struct list_elem *e;
    if(frame == victim) {
        if(list_size(&frame_list) == 1) victim = NULL;
        else {
            e = list_next(&victim->elem);
            if(e == list_end(&frame_list)) e = list_begin(&frame_list);
            victim = list_entry(e, struct frame_elem, elem);
        }
    }
    list_remove(&frame->elem);
    free(frame);
}

static bool swap_out_frame(struct frame_elem *frame) {
    size_t swap_num = swap_out(frame->kpage);
    if(swap_num == BITMAP_ERROR) return false;
    page_valid(frame->spt, frame->upage, true, swap_num);
    pagedir_clear_page(frame->pd, frame->upage);
	return true;
}

static void* get_victim_frame(void) {
    if(list_size(&frame_list) == 0) return NULL;
    struct frame_elem *e = NULL;
    struct list_elem *le = &victim->elem;
    void* kpage;
    size_t i;
    for(i = 0; i < list_size(&frame_list); i++, le = list_next(le)) {
        if(le == list_end(&frame_list)) le = list_begin(&frame_list);
        e = list_entry(le, struct frame_elem, elem);
        if(!pagedir_is_accessed(e->pd, e->upage)) break;
		pagedir_set_accessed(e->pd, e->upage, false);
    }
    if(list_next(le) == list_end(&frame_list)) victim = list_entry(list_begin(&frame_list), struct frame_elem, elem);
    else victim = list_entry(list_next(le), struct frame_elem, elem);
    kpage = e->kpage;
    swap_out_frame(e);
    free_frame(e);
    return kpage;
}

void* frame_alloc(void *upage, enum palloc_flags flags, bool writable) {
	//printf("(frame_alloc) upage : %p\n", upage);
	if(!is_user_vaddr(upage)) return NULL;
    lock_acquire(&frame_lock);
    void *kpage = palloc_get_page(flags);
    if(kpage == NULL) {
        kpage = get_victim_frame();
        if(kpage == NULL) return NULL;
    }
    uint32_t frame_num = palloc_user_page_number(kpage);
    struct frame_elem *e = get_frame(frame_num, true);
    if(e == NULL) {
        palloc_free_page(kpage);
        return NULL;
    }
    if(!pagedir_set_page(thread_current()->pagedir, upage, kpage, writable)) {
        free_frame(e);
        palloc_free_page(kpage);
        return NULL;
    }
    e->frame_num = frame_num;
    e->pd = thread_current()->pagedir;
    e->spt = &(thread_current()->spt);
    e->upage = upage;
    e->kpage = kpage;
    page_valid(e->spt, upage, false, e->frame_num);
    lock_release(&frame_lock);
    return kpage;
}

void frame_free(uint32_t frame_num) {
    lock_acquire(&frame_lock);
    struct frame_elem *frame = get_frame(frame_num, false);
    if(frame == NULL) return;
    page_invalid(frame->spt, frame->upage);
	palloc_free_page(frame->kpage);
    pagedir_clear_page(frame->pd, frame->upage);
    free_frame(frame);
    lock_release(&frame_lock);
}
