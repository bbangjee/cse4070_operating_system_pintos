#include "vm/page.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "userprog/process.h"
#include "userprog/pagedir.h"
#include <string.h>
#include "vm/frame.h"

extern struct lock filesys_lock;

// vm_entry_hash_func
static unsigned vm_entry_hash_func(const struct hash_elem* e, void* aux UNUSED) {
  // hash_entry()로 vm_entry 구조체 검색
  // hash_int()로 vaddr에 대한 해시값 구하기
  struct vm_entry* target = hash_entry(e, struct vm_entry, hash_elem);
  return hash_int((int)target->vaddr);
}

// vm_entry_less_func
static bool vm_entry_less_func(const struct hash_elem* a,
                               const struct hash_elem* b, void* aux UNUSED) {
  // hash_entry()로 vm_entry 구조체 얻고 vaddr 비교
  struct vm_entry* target_a = hash_entry(a, struct vm_entry, hash_elem);
  struct vm_entry* target_b = hash_entry(b, struct vm_entry, hash_elem);

  return target_a->vaddr < target_b->vaddr;
}


static void vm_entry_destroy_func(struct hash_elem *e, void* aux UNUSED) {
  struct vm_entry *vme = hash_entry(e, struct vm_entry, hash_elem);

  if (vme->is_loaded) {
    void *kpage = pagedir_get_page(thread_current()->pagedir, vme->vaddr);
    if (kpage != NULL) {
      free_page(kpage);
      pagedir_clear_page(thread_current()->pagedir, vme->vaddr);
    }
  }

  if (vme->type == VM_ANON && vme->swap_slot != 0) {
    swap_free(vme->swap_slot);
  }

  if (vme->mapped_file != NULL) file_close(vme->mapped_file);

  free(vme);
}

// vm_init
void vm_init(struct hash* vm) {
  // hash_init으로 해시 테이블 초기화
  // 인자로 vm_hash_func & vm_less_func
  hash_init(vm, vm_entry_hash_func, vm_entry_less_func, NULL);
}

void vm_destroy(struct hash* vm) {
  // 해시 테이블을 제거함
  hash_destroy(vm, vm_entry_destroy_func);
}

// insert
bool insert_vm_entry(struct hash* vm, struct vm_entry* ve) {
  // hash_insert() 사용해서 해시 테이블에 삽입
  struct hash_elem *temp = hash_insert(vm, &ve->hash_elem);
  if (temp == NULL) return true;
  return false;
}

// delete
bool delete_vm_entry(struct hash* vm, struct vm_entry* ve) {
  // hash_delete()
  struct hash_elem *e = hash_delete(vm, &ve->hash_elem);
  if (e == NULL) return false;

  struct vm_entry *found = hash_entry(e, struct vm_entry, hash_elem);
  free(found);
  return true;
}

// find
struct vm_entry* find_vm_entry(void *vaddr) {
  // demand paging에서 사용
  // pg_round_down() 이용해서 가상 메모리 주소에 해당하는 페이지 번호 추출
  // hash_find() 사용해서 hash_elem 얻기
  struct vm_entry temp;
  temp.vaddr = pg_round_down(vaddr);
  struct thread *cur = thread_current();
  struct hash_elem *e = hash_find(&cur->vm, &temp.hash_elem);
  if (e == NULL) return NULL;

  return hash_entry(e, struct vm_entry, hash_elem);
}

bool load_file(void *kaddr, struct vm_entry *vme) {
  // Disk에 존재하는 page를 물리 메모리로 1oad하는 함수
  // vme의 <파일,offset>으로 한 페이지를 kaddr로 읽어 들이는 함수를 구현
  // file_read_at() 함수 또는 file_read() + file_seek() 함수 이용
  // 4KB를 전부 write하지 못했다면 나머지를 0으로 채움
  lock_acquire(&filesys_lock);

  int bytes_read = file_read_at(vme->mapped_file, kaddr, vme->read_bytes, vme->offset);

  lock_release(&filesys_lock);

  if ((int)vme->read_bytes != bytes_read) {
    return false;
  }
  
  if (vme->zero_bytes > 0) memset(kaddr + vme->read_bytes, 0, vme->zero_bytes);
  return true;
}

bool expand_stack(void* vaddr) {

  void* page_addr = pg_round_down(vaddr);

  struct frame_entry *fe = alloc_page(PAL_USER | PAL_ZERO);
  if (fe == NULL) return false;
  void *kpage = fe->kaddr;

  // vme 생성 및 초기화
  struct vm_entry* vme = malloc(sizeof(struct vm_entry));
  if (vme == NULL) {
    free_page(kpage);
    return false;
  }
  vme->type = VM_ANON;
  vme->vaddr = page_addr;
  vme->writable = true;
  vme->is_loaded = true;  // 스택은 즉시 로드됨
  vme->mapped_file = NULL;
  vme->offset = 0;
  vme->read_bytes = 0;
  vme->zero_bytes = 0;
  vme->swap_slot = 0;

  fe->vme = vme;

  if (!install_page(page_addr, kpage, true)) {
    free_page(kpage);
    free(vme);
    return false;
  }
  
  if (!insert_vm_entry(&thread_current()->vm, vme)) {
    free_page(kpage);
    pagedir_clear_page(thread_current()->pagedir, page_addr);
    free(vme);
    return false;
  }

  return true;
}

bool verify_stack(void* vaddr, void *esp) {
  void *stack_bottom = (void *)((uint8_t *)PHYS_BASE - (8 * 1024 * 1024));

  if (vaddr >= PHYS_BASE) return false;
  if (vaddr < stack_bottom) return false;

  if (vaddr >= (uint8_t *)esp - 32) return true;

  return false;
}
