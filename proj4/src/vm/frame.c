#include "vm/frame.h"
#include "vm/page.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/synch.h"
#include "userprog/pagedir.h"
#include <stdio.h>

static struct list lru_list;
static struct lock lru_lock;
static struct list_elem *lru_clock;

extern struct lock filesys_lock;

void lru_list_init(void) {
  list_init(&lru_list);
  lock_init(&lru_lock);
  lru_clock = NULL;
}

/* LRU 리스트 관리 */
void add_page_to_lru_list(struct frame_entry *fe) {
  lock_acquire(&lru_lock);
  list_push_back(&lru_list, &fe->lru_elem);
  lock_release(&lru_lock);
}

void delete_page_from_lru_list(struct frame_entry *fe) {
  lock_acquire(&lru_lock);

  if (lru_clock == &fe->lru_elem) {
    lru_clock = list_next(lru_clock);
    if (lru_clock == list_end(&lru_list)) {
      lru_clock = list_begin(&lru_list);
    }
  }

  list_remove(&fe->lru_elem);
  lock_release(&lru_lock);
}

/* page 할당 및 해제 */
struct frame_entry * alloc_page(enum palloc_flags flags) {
  // lock_acquire(&lru_lock);

  void *kaddr = palloc_get_page(flags);

  while (kaddr == NULL) {
    // lock_release(&lru_lock);
    try_to_free_pages(flags);
    kaddr = palloc_get_page(flags);
  }

  lock_acquire(&lru_lock);

  struct frame_entry *fe = malloc(sizeof(struct frame_entry));
  if (fe == NULL) {
    palloc_free_page(kaddr);
    lock_release(&lru_lock);
    return NULL;
  }
  fe-> kaddr = kaddr;
  fe->t = thread_current();
  fe->vme = NULL;

  list_push_back(&lru_list, &fe->lru_elem);

  lock_release(&lru_lock);
  return fe;
}

void free_page(void *kaddr) {
  lock_acquire(&lru_lock);

  for (struct list_elem *e = list_begin(&lru_list); e != list_end(&lru_list); e = list_next(e)) {
    struct frame_entry *fe = list_entry(e, struct frame_entry, lru_elem);

    if (fe->kaddr == kaddr) {
      _free_page(fe);
      lock_release(&lru_lock);
      return;
    }
  }
  lock_release(&lru_lock);
}

void _free_page(struct frame_entry *fe) {
  list_remove(&fe->lru_elem);
  palloc_free_page(fe->kaddr);
  free(fe);
}

/* eviction */
void try_to_free_pages(enum palloc_flags flags) {
  lock_acquire(&lru_lock);
  while (true) {
    if (list_empty(&lru_list)) {
      lock_release(&lru_lock);
      return;
    }

    if (lru_clock == NULL || lru_clock == list_end(&lru_list)) {
      lru_clock = list_begin(&lru_list);
    }

    struct list_elem *e = lru_clock;
    lru_clock = list_next(lru_clock);

    struct frame_entry *victim = list_entry(e, struct frame_entry, lru_elem);

    if (pagedir_is_accessed(victim->t->pagedir, victim->vme->vaddr)) {
      pagedir_set_accessed(victim->t->pagedir, victim->vme->vaddr, false);
      continue;
    }
    
    list_remove(&victim->lru_elem);

    bool dirty = pagedir_is_dirty(victim->t->pagedir, victim->vme->vaddr);

    lock_release(&lru_lock);

    switch(victim->vme->type) {
      case VM_ANON:
        victim->vme->swap_slot = swap_out(victim->kaddr);
        break;
      case VM_FILE:
        if (dirty) {
          lock_acquire(&filesys_lock);
          file_write_at(victim->vme->mapped_file, victim->kaddr, victim->vme->read_bytes, victim->vme->offset);
          lock_release(&filesys_lock);
        }
        break;
      case VM_BINARY:
        if (dirty) {
          victim->vme->swap_slot = swap_out(victim->kaddr);
          victim->vme->type = VM_ANON; 
        }
        break;
    }
    pagedir_clear_page(victim->t->pagedir, victim->vme->vaddr);
    victim->vme->is_loaded = false;
    palloc_free_page(victim->kaddr);
    free(victim);

    // lock_acquire(&lru_lock);
    return;
  }

}

