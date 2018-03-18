#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <bitmap.h>
#include <hash.h>
#include "threads/synch.h"
#include "threads/palloc.h"
#include "threads/vaddr.h"
#include "devices/block.h"
#include "vm/swap.h"
#include "vm/frame.h"

#define SECTORS_PER_PAGE (PGSIZE / BLOCK_SECTOR_SIZE)

struct bitmap *swap_map;
struct block *swap_device; // device of BLOCK_SWAP
struct lock swap_lock;

/* Initiallize swap bitmap */
void swap_init(void) {
	swap_device = block_get_role(BLOCK_SWAP);
	swap_map = bitmap_create(block_size(swap_device));
	lock_init(&swap_lock);
}

/* Swap out frame */
size_t swap_out(void *page) {
	lock_acquire(&swap_lock);
	size_t swap_num = bitmap_scan_and_flip(swap_map, 0, SECTORS_PER_PAGE, false);
	if(swap_num == BITMAP_ERROR) return BITMAP_ERROR;
	int i;
	for(i = 0; i < SECTORS_PER_PAGE; i++)
		block_write(swap_device, swap_num + i, page + BLOCK_SECTOR_SIZE * i);
	lock_release(&swap_lock);
	return swap_num;
}

/* Swap in frame */
bool swap_in(size_t swap_num, void *page) {
	lock_acquire(&swap_lock);
	if(!bitmap_all(swap_map, swap_num, SECTORS_PER_PAGE)) return false;
	int i;
	//printf("(swap_in) swap_num : %d / kpage : %p\n", swap_num, page);
	for(i = 0; i < SECTORS_PER_PAGE; i++) block_read(swap_device, swap_num + i, page + BLOCK_SECTOR_SIZE * i);
	bitmap_set_multiple(swap_map, swap_num, SECTORS_PER_PAGE, false);
	lock_release(&swap_lock);
	return true;
}

/* Remove frame in the swap space, this function would be called
   when the thread which has a page in the swap space goes to die. */
void swap_free(size_t swap_num) {
	lock_acquire(&swap_lock);
	bitmap_set_multiple(swap_map, swap_num, SECTORS_PER_PAGE, false);
	lock_release(&swap_lock);
}
