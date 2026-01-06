#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <stdbool.h>
#include "vm/page.h"

void syscall_init (void);
struct vm_entry* check_address(void *vaddr, void *esp);
void sys_halt(void);
void sys_exit (int status);
int sys_exec(const char *cmd_line);
int sys_wait(int pid);
int sys_read(int fd, void *buffer, unsigned size);
int sys_write(int fd, const void *buffer, unsigned size);
int fibonacci(int n);
int max_of_four_int(int a, int b, int c, int d);
bool sys_create(const char *file, unsigned initial_size);
bool sys_remove(const char *file);
int sys_open(const char *file);
void sys_close(int fd);
int sys_filesize(int fd);
void sys_seek(int fd, unsigned position);
unsigned sys_tell(int fd);
void check_valid_buffer(void *buffer, unsigned size, void *esp, bool to_write);
void check_valid_string(const char *str, void* esp);
#endif /* userprog/syscall.h */
