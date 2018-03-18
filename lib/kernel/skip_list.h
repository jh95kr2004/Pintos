#ifndef __LIB_KERNEL_SKIP_LIST_H
#define __LIB_KERNEL_SKIP_LIST_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* Skip list element. */
struct skip_list_elem {
	int level;
	struct skip_list_elem **next;
	struct skip_list_elem **prev;
};

/* Skip list. */
struct skip_list {
	int level;
	struct skip_list_elem head;
	struct skip_list_elem tail;
};

#define skip_list_entry(SKIP_LIST_ELEM, STRUCT, MEMBER) ((STRUCT *) ((uint8_t *) &(SKIP_LIST_ELEM)->next - offsetof (STRUCT, MEMBER.next)))

typedef bool skip_list_less_func (const struct skip_list_elem *a, const struct skip_list_elem *b, void *aux);

/* Skip list initialize */
void skip_list_init (struct skip_list *);

/* Skip list traversal. */
struct skip_list_elem *skip_list_begin (struct skip_list *, int level);
struct skip_list_elem *skip_list_next (struct skip_list_elem *, int level);
struct skip_list_elem *skip_list_end (struct skip_list *);
struct skip_list_elem *skip_list_head (struct skip_list *);
struct skip_list_elem *skip_list_tail (struct skip_list *);

/* Skip list insertion. */
void skip_list_insert (struct skip_list *, struct skip_list_elem *, skip_list_less_func *, void *aux);

/* Skip list removal. */
struct skip_list_elem *skip_list_remove (struct skip_list *, struct skip_list_elem *);
struct skip_list_elem *skip_list_pop_front (struct skip_list *);

/* Skip list properties. */
size_t skip_list_size (struct skip_list *, int level);
bool skip_list_empty (struct skip_list *, int level);

/* Max and min. */
struct skip_list_elem *skip_list_max (struct skip_list *, skip_list_less_func *, void *aux);
struct skip_list_elem *skip_list_min (struct skip_list *, skip_list_less_func *, void *aux);

/* Search */
struct skip_list_elem *skip_list_search (struct skip_list *, struct skip_list_elem *, skip_list_less_func *, void *aux);

void skip_list_destroy(struct skip_list *);

#endif /* lib/kernel/skip_list.h */
