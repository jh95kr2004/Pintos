#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <stdbool.h>

typedef int pid_t;

void syscall_init (void);

void halt(void);
void exit(int);
pid_t exec(const char *);
int wait(pid_t);
bool create(const char*, unsigned);
bool remove(const char*);
int open(const char *);
int filesize(int);
int read(int, void*, unsigned);
int write(int, void*, unsigned);
void seek(int, unsigned);
unsigned tell(int);
void close(int);
int fibonacci(int);
int sum4int(int, int, int, int);

#endif /* userprog/syscall.h */
