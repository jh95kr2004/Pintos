#include "skip_list.h"
#include "../random.h"
#include "../debug.h"
#include "devices/rtc.h"
#include "threads/malloc.h"

static inline bool is_head (struct skip_list_elem *elem) {
  return elem != NULL && elem->prev == NULL && elem->next != NULL;
}

static inline bool is_interior (struct skip_list_elem *elem) {
  return elem != NULL && elem->prev != NULL && elem->next != NULL;
}

static inline bool is_tail (struct skip_list_elem *elem) {
  return elem != NULL && elem->next == NULL;
}

void skip_list_init (struct skip_list *list) {
  ASSERT (list != NULL);
  list->level = 0;
  list->head.level = 0;
  list->head.next = malloc(sizeof(struct skip_list_elem*));
  list->head.next[0] = &list->tail;
  list->head.prev = NULL;
  list->tail.level = 0;
  list->tail.next = NULL;
  list->tail.prev = malloc(sizeof(struct skip_list_elem*));
  list->tail.prev[0] = &list->head;
}

struct skip_list_elem *skip_list_begin (struct skip_list *list, int level) {
  ASSERT (list != NULL);
  ASSERT (list->head.next != NULL);
  return list->head.next[level];
}

struct skip_list_elem *skip_list_next (struct skip_list_elem *elem, int level) {
  ASSERT ((is_head (elem) || is_interior (elem)) && level >= 0);
  ASSERT (elem->next != NULL);
  return elem->next[level];
}

struct skip_list_elem *skip_list_end (struct skip_list *list) {
  ASSERT (list != NULL);
  return &list->tail;
}

struct skip_list_elem *skip_list_head (struct skip_list *list) {
  ASSERT (list != NULL);
  return &list->head;
}

struct skip_list_elem *skip_list_tail (struct skip_list *list) {
  ASSERT (list != NULL);
  return &list->tail;
}

void skip_list_insert (struct skip_list *list, struct skip_list_elem *elem, skip_list_less_func *less, void *aux) {
  ASSERT (list != NULL);
  ASSERT (elem != NULL);

  struct skip_list_elem *e, *next;
  int i, level, max_level = 0;

  random_init(rtc_get_time());
  while(random_ulong() % 2 == 0) max_level++;
  elem->level = max_level;
  elem->next = malloc(sizeof(struct skip_list_elem*)*(max_level+1));
  elem->prev = malloc(sizeof(struct skip_list_elem*)*(max_level+1));
  if(list->level < max_level) {
	  list->head.level = list->tail.level = max_level;
      list->head.next = realloc(list->head.next, sizeof(struct skip_list_elem*)*(max_level+1));
      list->tail.prev = realloc(list->tail.prev, sizeof(struct skip_list_elem*)*(max_level+1));
      ASSERT(list->head.next != NULL);
      ASSERT(list->tail.prev != NULL);
      for(i = list->level + 1; i <= max_level; i++) {
          list->head.next[i] = &list->tail;
          list->tail.prev[i] = &list->head;
      }
      list->level = max_level;
  }

  e = &list->head;
  level = list->level;
  while(level >= 0) {
      next = e->next[level];
      if(next == &list->tail || !less(next, elem, aux)) {
          if(max_level >= level) {
              elem->next[level] = next;
              elem->prev[level] = e;
			  e->next[level]->prev[level] = elem;
              e->next[level] = elem;
          }
          level--;
      } else e = next;
  }
}

struct skip_list_elem *skip_list_remove (struct skip_list *list, struct skip_list_elem *elem) {

  ASSERT (is_interior (elem));
  int i, cnt = 0;
  struct skip_list_elem *ret = elem->next[0];
  for(i = 0; i <= elem->level; i++) {
      elem->prev[i]->next[i] = elem->next[i];
      elem->next[i]->prev[i] = elem->prev[i];
  }
  free(elem->prev);
  free(elem->next);
  elem->level = -1;
  
  for(i = list->level; i >= 0; i--) {
      if(!skip_list_empty(list, i)) break;
      cnt++;
  }
  if(cnt > 0) {
      list->level = list->level - cnt;
      if(list->level < 0) {
          free(list->head.next);
          free(list->tail.prev);
          skip_list_init(list);
      }
      else {
          list->head.level = list->tail.level = list->level;
          list->head.next = realloc(list->head.next, sizeof(struct skip_list_elem*)*(list->level+1));
          list->tail.prev = realloc(list->tail.prev, sizeof(struct skip_list_elem*)*(list->level+1));
      }
  }
  return ret;
}

struct skip_list_elem *skip_list_pop_front (struct skip_list *list) {
	struct skip_list_elem *e = list->head.next[0];
	skip_list_remove(list, e);
	return e;
}

size_t skip_list_size (struct skip_list *list, int level) {
  struct skip_list_elem *e;
  size_t cnt = 0;

  for (e = list->head.next[level]; e != &list->tail; e = skip_list_next(e, level)) cnt++;
  return cnt;
}

bool skip_list_empty (struct skip_list *list, int level) {
  return list->head.next[level] == &list->tail;
}

struct skip_list_elem *skip_list_max (struct skip_list *list, skip_list_less_func *less, void *aux) {
  struct skip_list_elem *max = list->head.next[0];
  if (max != &list->tail) {
      struct skip_list_elem *e;
      for (e = skip_list_next(max, 0); e != &list->tail; e = skip_list_next(e, 0))
        if (less(max, e, aux))
          max = e; 
  }
  return max;
}

struct skip_list_elem *skip_list_min (struct skip_list *list, skip_list_less_func *less, void *aux) {
  struct skip_list_elem *min = list->head.next[0];
  if (min != &list->tail) {
      struct skip_list_elem *e;
      for (e = skip_list_next(min, 0); e != &list->tail; e = skip_list_next(e, 0))
        if (less(e, min, aux))
          min = e; 
  }
  return min;
}

struct skip_list_elem *skip_list_search (struct skip_list *list, struct skip_list_elem *elem, skip_list_less_func *less, void *aux) {
	struct skip_list_elem *e = &list->head, *next;
	int level = list->level;
	while(level >= 0) {
		next = skip_list_next(e, level);
		if(next == &list->tail || less(elem, next, aux)) level--;
		else {
			if(!less(next, elem, aux)) return next;
			e = next;
		}
	}
	return NULL;
}

void skip_list_destroy(struct skip_list *list) {
	if(list->head.next != NULL) free(list->head.next);
	if(list->tail.prev != NULL) free(list->tail.prev);
}
