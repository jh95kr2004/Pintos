#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <skip_list.h>
#include <hash.h>
#include <stdint.h>
#include "threads/synch.h"

/* For priority aging. */
extern bool thread_prior_aging;

/* States in a thread's life cycle. */
enum thread_status
  {
    THREAD_RUNNING,     /* Running thread. */
    THREAD_READY,       /* Not running but ready to run. */
    THREAD_BLOCKED,     /* Waiting for an event to trigger. */
    THREAD_DYING        /* About to be destroyed. */
  };

/* Thread identifier type.
   You can redefine this to whatever type you like. */
typedef int tid_t;
#define TID_ERROR ((tid_t) -1)          /* Error value for tid_t. */

/* Thread priorities. */
#define PRI_MIN 0                       /* Lowest priority. */
#define PRI_DEFAULT 31                  /* Default priority. */
#define PRI_MAX 63                      /* Highest priority. */

#define NICE_MIN -20
#define NICE_DEFAULT 0
#define NICE_MAX 20

/* A kernel thread or user process.

   Each thread structure is stored in its own 4 kB page.  The
   thread structure itself sits at the very bottom of the page
   (at offset 0).  The rest of the page is reserved for the
   thread's kernel stack, which grows downward from the top of
   the page (at offset 4 kB).  Here's an illustration:

        4 kB +---------------------------------+
             |          kernel stack           |
             |                |                |
             |                |                |
             |                V                |
             |         grows downward          |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             +---------------------------------+
             |              magic              |
             |                :                |
             |                :                |
             |               name              |
             |              status             |
        0 kB +---------------------------------+

   The upshot of this is twofold:

      1. First, `struct thread' must not be allowed to grow too
         big.  If it does, then there will not be enough room for
         the kernel stack.  Our base `struct thread' is only a
         few bytes in size.  It probably should stay well under 1
         kB.

      2. Second, kernel stacks must not be allowed to grow too
         large.  If a stack overflows, it will corrupt the thread
         state.  Thus, kernel functions should not allocate large
         structures or arrays as non-static local variables.  Use
         dynamic allocation with malloc() or palloc_get_page()
         instead.

   The first symptom of either of these problems will probably be
   an assertion failure in thread_current(), which checks that
   the `magic' member of the running thread's `struct thread' is
   set to THREAD_MAGIC.  Stack overflow will normally change this
   value, triggering the assertion. */
/* The `elem' member has a dual purpose.  It can be an element in
   the run queue (thread.c), or it can be an element in a
   semaphore wait list (synch.c).  It can be used these two ways
   only because they are mutually exclusive: only a thread in the
   ready state is on the run queue, whereas only a thread in the
   blocked state is on a semaphore wait list. */
struct thread
  {
    /* Owned by thread.c. */
    tid_t tid;                          /* Thread identifier. */
    enum thread_status status;          /* Thread state. */
    char name[16];                      /* Name (for debugging purposes). */
    uint8_t *stack;                     /* Saved stack pointer. */
    int priority;                       /* Priority. */
    struct list_elem allelem;           /* List element for all threads list. */

	/* BSD Scheduler */
	int nice;
	int recent_cpu;

    /* Shared between thread.c and synch.c. */
    struct list_elem elem;              /* List element. */

#ifdef USERPROG
    /* Owned by userprog/process.c. */
    uint32_t *pagedir;                  /* Page directory. */
#endif

    /* Owned by thread.c. */
    unsigned magic;                     /* Detects stack overflow. */

	/* For synchronize parent and childs (wait) */
	struct thread *parent;				/* Parent thread */
	struct skip_list *wait_list;		/* Skip list to get exit status of child threads */
	struct wait_elem *wait_elem;	    /* Wait elem that parent thread has */

    /* For file system */
#ifdef FILESYS
	struct skip_list *fd_list;           /* Skip list of file descriptors */
#endif

	/* For prevent modifying excutable file */
	struct file *exec;					 /* Executable file of this thread */

	/* For synchronization */
	struct semaphore sema;

	/* For timer sleep */
	int64_t ticks;

	/* For VM */
	struct hash spt;					 /* Supplemental page table */
	void *esp;							 /* Stack pointer */
  };

/* Parent thread has skip list that uses this wait_elem struct
   Via this struct parent can get exit status of child threads
   even if child threads are terminated before parent wait */
struct wait_elem {
    tid_t tid;                           /* tid of wait child */
    struct thread *child;
    int exit_status;					 /* Exit status of thread */
	bool load_flag;						 /* If child successes to load, load_flag = true */
    bool exit_flag;                      /* If child thread exit normally, exit_flag = true, otherwise exit_flag = false */
	bool wait_flag;				 		 /* If parent wait this child, wait_flag is true */
	bool orphan_flag;					 /* If parent thread dies before wait child thread, child thread is orphan */
    struct skip_list_elem elem;          /* Skip list element for wait_list */
	struct semaphore load_sema;			 /* Semaphore to validate the load */
};

/* For donate priority */
/*struct lock_elem {
	struct lock* lock;
	struct list_elem elem;
};*/

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
extern bool thread_mlfqs;

void thread_init (void);
void thread_start (void);

void thread_tick (void);
void thread_print_stats (void);

typedef void thread_func (void *aux);
tid_t thread_create (const char *name, int priority, thread_func *, void *);

void thread_block (void);
void thread_unblock (struct thread *);

struct thread *thread_current (void);
tid_t thread_tid (void);
const char *thread_name (void);

void thread_exit (void) NO_RETURN;
void thread_yield (void);

/* Performs some operation on thread t, given auxiliary data AUX. */
typedef void thread_action_func (struct thread *t, void *aux);
void thread_foreach (thread_action_func *, void *);

int thread_get_priority (void);
void thread_set_priority (int);

int thread_get_priority_of (struct thread *);
void thread_set_priority_of (struct thread *, int);

int thread_get_nice (void);
void thread_set_nice (int);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);

/* Functions for wait child */
struct wait_elem *thread_get_wait_elem(tid_t, struct thread *);
void thread_remove_wait_elem(struct thread *, struct wait_elem *);

/* For timer sleep */
void thread_sleep(int64_t);
void thread_awake(struct thread *);

#endif /* threads/thread.h */
