#include "userprog/syscall.h"
#include "userprog/process.h"
#include <stdio.h>
#include <string.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/synch.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "filesys/filesys.h"
#include "filesys/file.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
  if(f->esp == NULL || f->esp < 0x08048000 || f->esp >= PHYS_BASE) exit(-1);
  int syscallnum = *((int*)(f->esp));
  thread_current()->esp = f->esp;
  switch(syscallnum) {
	  case SYS_HALT:
		  halt();
		  break;
	  case SYS_EXIT:
		  if(f->esp > PHYS_BASE - 2 * sizeof(uintptr_t)) exit(-1);
		  exit(*(int*)(f->esp+sizeof(uintptr_t)));
		  break;
	  case SYS_EXEC:
		  if(f->esp > PHYS_BASE - 2 * sizeof(uintptr_t)) exit(-1);
		  f->eax = exec(*(char**)(f->esp+sizeof(uintptr_t)));
		  break;
	  case SYS_WAIT:
		  if(f->esp > PHYS_BASE - 2 * sizeof(uintptr_t)) exit(-1);
		  f->eax = wait(*(pid_t*)(f->esp+sizeof(uintptr_t)));
		  break;
	  case SYS_CREATE:
		  if(f->esp + 12 > PHYS_BASE - 3 * sizeof(uintptr_t)) exit(-1);
		  f->eax = create(*(char**)(f->esp+12+sizeof(uintptr_t)), *(unsigned*)(f->esp+12+2*sizeof(uintptr_t)));
		  break;
	  case SYS_REMOVE:
		  if(f->esp > PHYS_BASE - 2 * sizeof(uintptr_t)) exit(-1);
		  f->eax = remove(*(char**)(f->esp+sizeof(uintptr_t)));
		  break;
	  case SYS_OPEN:
		  if(f->esp > PHYS_BASE - 2 * sizeof(uintptr_t)) exit(-1);
		  f->eax = open(*(char**)(f->esp+sizeof(uintptr_t)));
		  break;
	  case SYS_FILESIZE:
		  if(f->esp > PHYS_BASE - 2 * sizeof(uintptr_t)) exit(-1);
		  f->eax = filesize(*(int*)(f->esp+sizeof(uintptr_t)));
		  break;
	  case SYS_READ:
		  if(f->esp + 16 > PHYS_BASE - 4 * sizeof(uintptr_t)) exit(-1);
		  f->eax = read(*(int*)(f->esp+16+sizeof(uintptr_t)), *(void**)(f->esp+16+2*sizeof(uintptr_t)), *(unsigned*)(f->esp+16+3*sizeof(uintptr_t)));
		  break;
	  case SYS_WRITE:
		  if(f->esp + 16 > PHYS_BASE - 4 * sizeof(uintptr_t)) exit(-1);
		  f->eax = write(*(int*)(f->esp+16+sizeof(uintptr_t)), *(void**)(f->esp+16+2*sizeof(uintptr_t)), *(unsigned*)(f->esp+16+3*sizeof(uintptr_t)));
		  break;
	  case SYS_SEEK:
		  if(f->esp + 12 > PHYS_BASE - 3 * sizeof(uintptr_t)) exit(-1);
		  seek(*(int*)(f->esp+12+sizeof(uintptr_t)), *(unsigned*)(f->esp+12+2*sizeof(uintptr_t)));
		  break;
	  case SYS_TELL:
		  if(f->esp > PHYS_BASE - 2 * sizeof(uintptr_t)) exit(-1);
		  f->eax = tell(*(int*)(f->esp+sizeof(uintptr_t)));
		  break;
	  case SYS_CLOSE:
		  if(f->esp > PHYS_BASE - 2 * sizeof(uintptr_t)) exit(-1);
		  close(*(int*)(f->esp+sizeof(uintptr_t)));
		  break;
	  case SYS_FIBONACCI:
		  if(f->esp > PHYS_BASE - 2 * sizeof(uintptr_t)) exit(-1);
		  f->eax = fibonacci(*(int*)(f->esp+sizeof(uintptr_t)));
		  break;
	  case SYS_SUM4INT:
		  if(f->esp + 20 > PHYS_BASE - 5 * sizeof(uintptr_t)) exit(-1);
		  f->eax = sum4int(*(int*)(f->esp+20+sizeof(uintptr_t)), *(int*)(f->esp+20+2*sizeof(uintptr_t)), *(int*)(f->esp+20+3*sizeof(uintptr_t)), *(int*)(f->esp+20+4*sizeof(uintptr_t)));
		  break;
	  default: break;
  }
}

void halt(void) {
	shutdown_power_off();
}

void exit(int status) {
	struct thread *cur = thread_current();
	char name[15];
	int i;

	/* get program name of thread */
	for(i = 0; i < strlen(cur->name); i++) {
		if(cur->name[i] == '\0' || cur->name[i] == ' ') break;
		name[i] = cur->name[i];
	}
	name[i] = '\0';

	/* print do exit status and the name of exit thread */
	printf("%s: exit(%d)\n", name, status);
	if(cur->wait_elem != NULL) {
        cur->wait_elem->exit_status = status;
        cur->wait_elem->exit_flag = true;
    }
	thread_exit();
}

pid_t exec(const char *cmd_line) {
	return process_execute(cmd_line);
}

int wait(pid_t pid){
	return process_wait(pid);
}

bool create(const char *file, unsigned initial_size) {
	if(file == NULL) exit(-1);
	return filesys_create(file, initial_size);
}

bool remove(const char *file) {
	if(file == NULL) exit(-1);
	return filesys_remove(file);
}

int open(const char *file) {
	if(file == NULL) exit(-1);
	struct file* res = filesys_open(file);
	if(res == NULL) return -1;
	return file_new_fd(res, thread_current());
}

int filesize(int fd) {
	struct file *file = file_of_fd(fd, thread_current());
	if(file == NULL) exit(-1);
	return (int)file_length(file);
}

int read(int fd, void *buffer, unsigned size) {
	if(size == 0) return 0;
	if(buffer >= PHYS_BASE || put_user(buffer, size) == -1) exit(-1);
	if(fd == 0) {
		unsigned int i;
		for(i = 0; i < size; i++){
			*((char*)(buffer + i)) = input_getc();
			if(*((char*)(buffer + i)) == '\n' || *((char*)(buffer+i)) == '\0') break;
		}
		*((char*)(buffer + i)) = '\0';
		return i;
	} else {
		struct file *file = file_of_fd(fd, thread_current());
		if(file == NULL) exit(-1);
		return file_read(file, buffer, size);
	}
}

int write(int fd, void *buffer, unsigned size) {
	if(size == 0) return 0;
	if(buffer >= PHYS_BASE || get_user(buffer) == -1) exit(-1);
	if(fd == 1) {
		putbuf(buffer, size);
		return size;
	} else {
		struct file *file = file_of_fd(fd, thread_current());
		if(file == NULL) exit(-1);
		return file_write(file, buffer, size);
	}
}

void seek(int fd, unsigned position) {
	struct file *file = file_of_fd(fd, thread_current());
	if(file == NULL) exit(-1);
	file_seek(file, position);
}

unsigned tell(int fd) {
	struct file *file = file_of_fd(fd, thread_current());
	if(file == NULL) exit(-1);
	return (unsigned)file_tell(file);
}

void close(int fd) {
	struct file *file = file_of_fd(fd, thread_current());
	if(file == NULL) exit(-1);
	file_close(file);
	file_remove_fd(fd, thread_current());
}

int fibonacci(int n) {
	int a = 1, b = 1, temp, i;
	for(i = 3; i <= n; i++) {
		temp = b;
		b = a + b;
		a = temp;
	}
	return b;
}

int sum4int(int a, int b, int c, int d) {
	return a + b + c + d;
}
