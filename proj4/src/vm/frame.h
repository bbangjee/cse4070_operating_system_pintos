#ifndef VM_FRAME_H
#define VM_FRAME_H

#include <list.h>
#include "threads/palloc.h"

struct frame_entry {
  void* kaddr;           /* 물리 페이지 주소 (커널 가상 주소) */
  struct thread* t;      /* 이 페이지를 소유한 스레드 */
  struct vm_entry* vme;  /* 이 페이지에 매핑된 vm_entry */
  struct list_elem lru_elem; /* 프레임 테이블 리스트용 엘리먼트 */
};

// extern struct lock lru_lock;
// extern struct list lru_list;

/* 초기화 */
void lru_list_init(void);
/* LRU 리스트 관리 */
void add_page_to_lru_list(struct frame_entry *fe);
void delete_page_from_lru_list(struct frame_entry *fe);

/* page 할당 및 해제 */
struct frame_entry * alloc_page(enum palloc_flags flags);
void free_page(void *kaddr);
void _free_page(struct frame_entry *fe);

/* eviction */
void try_to_free_pages(enum palloc_flags flags);

#endif