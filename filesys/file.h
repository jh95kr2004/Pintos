#ifndef FILESYS_FILE_H
#define FILESYS_FILE_H

#include "threads/thread.h"
#include "filesys/off_t.h"

struct inode;

/* Opening and closing files. */
struct file *file_open (struct inode *);
struct file *file_reopen (struct file *);
void file_close (struct file *);
struct inode *file_get_inode (struct file *);

/* Reading and writing. */
off_t file_read (struct file *, void *, off_t);
off_t file_read_at (struct file *, void *, off_t size, off_t start);
off_t file_write (struct file *, const void *, off_t);
off_t file_write_at (struct file *, const void *, off_t size, off_t start);

/* Preventing writes. */
void file_deny_write (struct file *);
void file_allow_write (struct file *);

/* File position. */
void file_seek (struct file *, off_t);
off_t file_tell (struct file *);
off_t file_length (struct file *);

/* file descriptor functions */
void file_fd_init(void);
void file_fd_done(void);
int file_new_fd(struct file*, struct thread*);
void file_remove_fd(int fd, struct thread*);
struct file* file_of_fd(int fd, struct thread*);

/* for threads */
void file_remove_all_fd(struct thread*);

/* for synchronization */
struct semaphore *file_sema(struct file *);

#endif /* filesys/file.h */
