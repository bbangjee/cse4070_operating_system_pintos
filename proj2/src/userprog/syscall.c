#include "userprog/syscall.h"

#include <stdint.h>
#include <stdio.h>
#include <syscall-nr.h>

#include "devices/input.h"  // input_getc()
#include "devices/shutdown.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"     // is_user_vaddr()
#include "userprog/pagedir.h"  // pagedir_get_page()
#include "userprog/process.h"
#include "threads/synch.h"  // struct lock
#include "filesys/file.h"    // struct file
#include "filesys/filesys.h"

struct lock filesys_lock;
static void syscall_handler(struct intr_frame *);

void syscall_init(void) {
  intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init(&filesys_lock);
}

static void syscall_handler(struct intr_frame *f) {
  uint32_t *esp = f->esp;
  check_address(esp);
  int syscall_number = *esp;

  // printf("System call: %d\n", *esp);

  switch (syscall_number) {
    case SYS_HALT:
      sys_halt();
      break;
    case SYS_EXIT:
      check_address(esp + 1);
      sys_exit((int)(*(esp + 1)));
      break;
    case SYS_EXEC:
      check_address(esp + 1);
      check_address(*(esp + 1));
      f->eax = sys_exec((const char *)(*(esp + 1)));
      break;
    case SYS_WAIT:
      check_address(esp + 1);
      f->eax = sys_wait((int)(*(esp + 1)));
      break;
    case SYS_READ:
      check_address(esp + 1);
      check_address(esp + 2);
      check_address(*(esp + 2));
      check_address(esp + 3);
      f->eax = sys_read((int)(*(esp + 1)), (const void *)(*(esp + 2)), (unsigned)(*(esp + 3)));
      break;
    case SYS_WRITE:
      check_address(esp + 1);
      check_address(esp + 2);
      check_address(*(esp + 2));
      check_address(esp + 3);
      f->eax = sys_write((int)(*(esp + 1)), (const void *)(*(esp + 2)), (unsigned)(*(esp + 3)));
      break;
    case SYS_FIBONACCI:
      check_address(esp + 1);
      f->eax = fibonacci((int)(*(esp + 1)));
      break;
    case SYS_MAX_OF_FOUR_INT:
      check_address(esp + 1);
      check_address(esp + 2);
      check_address(esp + 3);
      check_address(esp + 4);
      f->eax = max_of_four_int((int)(*(esp + 1)), (int)(*(esp + 2)), (int)(*(esp + 3)), (int)(*(esp + 4)));
      break;
    case SYS_CREATE:
      check_address(esp + 1);
      check_address(*(esp + 1));
      check_address(esp + 2);
      f->eax = sys_create((const char *)(*(esp + 1)), (unsigned)(*(esp + 2)));
      break;
    case SYS_REMOVE:
      check_address(esp + 1);
      check_address(*(esp + 1));
      f->eax = sys_remove((const char *)(*(esp + 1)));
      break;
    case SYS_OPEN:
      check_address(esp + 1);
      check_address(*(esp + 1));
      f->eax = sys_open((const char *)(*(esp + 1)));
      break;
    case SYS_CLOSE:
      check_address(esp + 1);
      sys_close((int)(*(esp + 1)));
      break;
    case SYS_FILESIZE:
      check_address(esp + 1);
      f->eax = sys_filesize((int)(*(esp + 1)));
      break;
    case SYS_SEEK:
      check_address(esp + 1);
      check_address(esp + 2);
      sys_seek((int)(*(esp + 1)), (unsigned)(*(esp + 2)));
      break;
    case SYS_TELL:
      check_address(esp + 1);
      f->eax = sys_tell((int)(*(esp + 1)));
      break;
  }
}

void sys_halt(void) { shutdown_power_off(); }

void sys_exit(int status) {
  struct thread *cur = thread_current();
  cur->exit_status = status;
  printf("%s: exit(%d)\n", thread_name(), status);
  thread_exit();
}

int sys_exec(const char *cmd_line) {
  return process_execute(cmd_line);
}

int sys_wait(int pid) { return process_wait(pid); }

int sys_read(int fd, void *buffer, unsigned size) {
  unsigned cnt = 0;
  char *buf = (char *)buffer;
  struct thread *cur = thread_current();
  // fd가 0인 경우 (표준 입력) ==> input_getc() 사용
  if (fd == 0) {
    while (cnt < size) {
      char c = input_getc();
      buf[cnt++] = c;
      // if (c == '\n') break;
    }
    // buf[cnt] = '\0';
    return cnt;
  }
  if (fd < 3 || fd >= 128 || cur->fdt[fd] == NULL) return -1;

  lock_acquire(&filesys_lock);
  int bytes = file_read(cur->fdt[fd], buffer, size);
  lock_release(&filesys_lock);
  return bytes;
}

int sys_write(int fd, const void *buffer, unsigned size) {
  // check_address(buffer);
  struct thread *cur = thread_current();
  if (fd == 1) {
    // printf("SYS_WRITE buffer content: %.*s\n", size, (char *)buffer);
    putbuf(buffer, size);
    return size;
  }
  if (fd < 3 || fd >= 128 || cur->fdt[fd] == NULL) return -1;
  lock_acquire(&filesys_lock);
  int bytes = file_write(cur->fdt[fd], buffer, size);
  lock_release(&filesys_lock);
  return bytes;
}

void check_address(void *vaddr) {
  struct thread *t = thread_current();
  if (vaddr == NULL || !is_user_vaddr(vaddr) || pagedir_get_page(t->pagedir, vaddr) == NULL) {
    // printf("Invalid address: %p\n", vaddr);
    sys_exit(-1);
  }
}

int fibonacci(int n) {
  if (n < 0) return 0;
  if (n <= 1) return n;
  int x0 = 0, x1 = 1;
  int res = 1;
  for (int i = 2; i <= n-1; i++) {
    x0 = x1;
    x1 = res;
    res = x0 + x1;
  }
  return res;
}

int max_of_four_int(int a, int b, int c, int d) {
  int max = a;
  if (b > max) max = b;
  if (c > max) max = c;
  if (d > max) max = d;
  return max;
}

bool sys_create(const char *file, unsigned initial_size) {
  lock_acquire(&filesys_lock);
  bool success = filesys_create(file, initial_size);
  lock_release(&filesys_lock);
  return success;
}

bool sys_remove(const char *file) {
  lock_acquire(&filesys_lock);
  bool success = filesys_remove(file);
  lock_release(&filesys_lock);
  return success;
}

int sys_open(const char *file) {
  struct thread *cur = thread_current();

  lock_acquire(&filesys_lock);
  struct file *f = filesys_open(file);
  lock_release(&filesys_lock);

  if (f == NULL) return -1;

  for (int fd = 3; fd < 128; fd++) {
    if (cur->fdt[fd] == NULL) {
      cur->fdt[fd] = f;
      return fd;
    }
  }
    
  lock_acquire(&filesys_lock);
  file_close(f);
  lock_release(&filesys_lock);
  return -1;
}

void sys_close(int fd) {
  struct thread *cur = thread_current();
  if (fd < 3 || fd >= 128 || cur->fdt[fd] == NULL) return;
  lock_acquire(&filesys_lock);
  file_close(cur->fdt[fd]);
  lock_release(&filesys_lock);
  cur->fdt[fd] = NULL;
}

int sys_filesize(int fd) {
  struct thread *cur = thread_current();
  if (fd < 3 || fd >= 128 || cur->fdt[fd] == NULL) return -1;
  lock_acquire(&filesys_lock);
  int size = file_length(cur->fdt[fd]);
  lock_release(&filesys_lock);
  return size;
}

void sys_seek(int fd, unsigned position) {
  struct thread *cur = thread_current();
  if (fd < 3 || fd >= 128 || cur->fdt[fd] == NULL) return;
  file_seek(cur->fdt[fd], position);
}

unsigned sys_tell(int fd) {
  struct thread *cur = thread_current();
  if (fd < 3 || fd >= 128 || cur->fdt[fd] == NULL) return -1;
  return file_tell(cur->fdt[fd]);
}
