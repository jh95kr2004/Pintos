#include "filesys/file.h"
#include <debug.h>
#include <list.h>
#include <skip_list.h>
#include "filesys/inode.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "threads/interrupt.h"
#include "threads/synch.h"
#ifdef USERPROG
#include "userprog/syscall.h"
#endif

/* file descriptor skip lists. */
struct list fd_trash_list;
int max_fd = 2;

struct fd {
	int fd;
	struct file* file;
	struct skip_list_elem elem;
	struct list_elem trash_elem;
};

struct lock fd_lock;

/* An open file. */
struct file 
  {
    struct inode *inode;        /* File's inode. */
    off_t pos;                  /* Current position. */
    bool deny_write;            /* Has file_deny_write() been called? */
  };

/* Opens a file for the given INODE, of which it takes ownership,
   and returns the new file.  Returns a null pointer if an
   allocation fails or if INODE is null. */
struct file *
file_open (struct inode *inode) 
{
  struct file *file = calloc (1, sizeof *file);
  if (inode != NULL && file != NULL)
    {
      file->inode = inode;
      file->pos = 0;
      file->deny_write = false;
      return file;
    }
  else
    {
      inode_close (inode);
      free (file);
      return NULL; 
    }
}

/* Opens and returns a new file for the same inode as FILE.
   Returns a null pointer if unsuccessful. */
struct file *
file_reopen (struct file *file) 
{
  return file_open (inode_reopen (file->inode));
}

/* Closes FILE. */
void
file_close (struct file *file) 
{
  if (file != NULL)
    {
      file_allow_write (file);
      inode_close (file->inode);
      free (file); 
    }
}

/* Returns the inode encapsulated by FILE. */
struct inode *
file_get_inode (struct file *file) 
{
  return file->inode;
}

/* Reads SIZE bytes from FILE into BUFFER,
   starting at the file's current position.
   Returns the number of bytes actually read,
   which may be less than SIZE if end of file is reached.
   Advances FILE's position by the number of bytes read. */
off_t
file_read (struct file *file, void *buffer, off_t size) 
{
  sema_down(file_sema(file));
  off_t readed_bytes = inode_read_at (file->inode, buffer, size, file->pos);
  file->pos += readed_bytes;
  sema_up(file_sema(file));
  return readed_bytes;
}

/* Reads SIZE bytes from FILE into BUFFER,
   starting at offset FILE_OFS in the file.
   Returns the number of bytes actually read,
   which may be less than SIZE if end of file is reached.
   The file's current position is unaffected. */
off_t
file_read_at (struct file *file, void *buffer, off_t size, off_t file_ofs) 
{
  return inode_read_at (file->inode, buffer, size, file_ofs);
}

/* Writes SIZE bytes from BUFFER into FILE,
   starting at the file's current position.
   Returns the number of bytes actually written,
   which may be less than SIZE if end of file is reached.
   (Normally we'd grow the file in that case, but file growth is
   not yet implemented.)
   Advances FILE's position by the number of bytes read. */
off_t
file_write (struct file *file, const void *buffer, off_t size) 
{
  sema_down(file_sema(file));
  off_t bytes_written = inode_write_at (file->inode, buffer, size, file->pos);
  file->pos += bytes_written;
  sema_up(file_sema(file));
  return bytes_written;
}

/* Writes SIZE bytes from BUFFER into FILE,
   starting at offset FILE_OFS in the file.
   Returns the number of bytes actually written,
   which may be less than SIZE if end of file is reached.
   (Normally we'd grow the file in that case, but file growth is
   not yet implemented.)
   The file's current position is unaffected. */
off_t
file_write_at (struct file *file, const void *buffer, off_t size,
               off_t file_ofs) 
{
  return inode_write_at (file->inode, buffer, size, file_ofs);
}

/* Prevents write operations on FILE's underlying inode
   until file_allow_write() is called or FILE is closed. */
void
file_deny_write (struct file *file) 
{
  ASSERT (file != NULL);
  if (!file->deny_write) 
    {
      file->deny_write = true;
      inode_deny_write (file->inode);
    }
}

/* Re-enables write operations on FILE's underlying inode.
   (Writes might still be denied by some other file that has the
   same inode open.) */
void
file_allow_write (struct file *file) 
{
  ASSERT (file != NULL);
  if (file->deny_write) 
    {
      file->deny_write = false;
      inode_allow_write (file->inode);
    }
}

/* Returns the size of FILE in bytes. */
off_t
file_length (struct file *file) 
{
  ASSERT (file != NULL);
  return inode_length (file->inode);
}

/* Sets the current position in FILE to NEW_POS bytes from the
   start of the file. */
void
file_seek (struct file *file, off_t new_pos)
{
  ASSERT (file != NULL);
  ASSERT (new_pos >= 0);
  file->pos = new_pos;
}

/* Returns the current position in FILE as a byte offset from the
   start of the file. */
off_t
file_tell (struct file *file) 
{
  ASSERT (file != NULL);
  return file->pos;
}

/* less function for skip list */
static bool less_fd_list(const struct skip_list_elem *a, const struct skip_list_elem *b, void *aux) {
	if((skip_list_entry(a, struct fd, elem)->fd) < (skip_list_entry(b, struct fd, elem)->fd)) return true;
	return false;
}

/* get skip list elem of fd */
static struct skip_list_elem* elem_of_fd(int fd, struct thread *t) {
    if(t->fd_list == NULL) return NULL;
	struct fd f;
	f.fd = fd;
	return skip_list_search(t->fd_list, &f.elem, less_fd_list, NULL);
}


/* initialize file descriptor */
void file_fd_init(void) {
	list_init(&fd_trash_list);
	lock_init(&fd_lock);
	max_fd = 2;
}

/* free all fds in trash list. It might be called when pintos terminates */
void file_fd_done(void) {
	struct list_elem *e;
	struct fd *f;
	while(!list_empty(&fd_trash_list)) {
		e = list_pop_front(&fd_trash_list);
		f = list_entry(e, struct fd, elem);
		free(f);
	}
}

/* make new file descriptor and return value of fd */
int file_new_fd(struct file* file, struct thread *t) {
	lock_acquire(&fd_lock);
	struct fd *f;
	if(list_empty(&fd_trash_list)) {
		f = (struct fd*)malloc(sizeof(struct fd));
		f->fd = max_fd++;
	}
	else f = list_entry(list_pop_front(&fd_trash_list), struct fd, trash_elem);
	f->file = file;
    if(t->fd_list == NULL) {
        t->fd_list = (struct skip_list*)malloc(sizeof(struct skip_list));
        skip_list_init(t->fd_list);
    }
	skip_list_insert(t->fd_list, &f->elem, less_fd_list, NULL);
	lock_release(&fd_lock);
	return f->fd;
}

/* remove file descriptor */
void file_remove_fd(int fd, struct thread *t) {
    if(t->fd_list == NULL) return;
	struct skip_list_elem *e = elem_of_fd(fd, t);
	if(e != NULL) {
		enum intr_level old_level = intr_disable();
		skip_list_remove(t->fd_list, e);
		list_push_back(&fd_trash_list, &(skip_list_entry(e, struct fd, elem)->trash_elem)); 	// insert removed fd into trash fd list.
		intr_set_level(old_level);
	}
}

/* get file pointer of file descriptor */
struct file* file_of_fd(int fd, struct thread *t) {
	struct skip_list_elem *e = elem_of_fd(fd, t);
	if(e == NULL) return NULL;
	return (skip_list_entry(e, struct fd, elem))->file;
}

/* move out all fds in thread. And close all files this thread opened.
   This might be called when thread is terminating */
void file_remove_all_fd(struct thread *t) {
	struct skip_list_elem *e;
	struct fd *f;
	if(t->fd_list == NULL || skip_list_empty(t->fd_list, 0)) return;
	enum intr_level old_level = intr_disable();
	for(e = skip_list_begin(t->fd_list, 0); e != skip_list_end(t->fd_list);) {
		f = list_entry(e, struct fd, elem);
		file_close(f->file);
		e = skip_list_remove(t->fd_list, e);
		list_push_back(&fd_trash_list, &f->trash_elem);
	}
	intr_set_level(old_level);
}

/* returns sema of file */
struct semaphore* file_sema(struct file *file) {
	return inode_sema(file->inode);
}
