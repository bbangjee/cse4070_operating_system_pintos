#ifndef VM_PAGE_H
#define VM_PAGE_H
#include <hash.h>
#include <debug.h>
#include <stdbool.h>
#include "filesys/file.h"

enum vm_type {
  // 바이너리 파일로부터 데이터를 로드
  // Evict 시 Read-Only면 버림, Writable이면 swap
  VM_BINARY,
  // 매핑된 파일로부터 데이터를 로드
  // mmap()된 파일, dirty면 write back
  VM_FILE,
  // 스왑영역으로부터 데이터를 로드
  // Stack/Heap/BSS, 반드시 swap disk에 저장
  VM_ANON
};

/********************************************************
 * page fault의 경우, vm_entry 탐색
 * vm_entry가 존재하면
 *    page frame 할당
 *    파일 포인터, 읽기 시작할 오프셋, 읽어야할 크기 참조해서 페이지 로드
 * 
 * vm_entry가 존재하지 않는 가상주소
 *    segmentation fault
 ********************************************************/ 

struct vm_entry {
  uint8_t type;    // VM_BINARY, VM_FILE, VM_ANON
  
  size_t offset;      // 읽어야 할 파일 offset
  size_t read_bytes;  // 가상페이지에 쓰여져 있는 데이터 크기
  size_t zero_bytes;  // 0으로 채울 남은 페이지의 byte

  bool writable;   // True이면 write 가능, False이면 write 불가능
  bool is_loaded;  // 물리 메모리 탑재 여부

  void* vaddr;     // vm_entry의 가상페이지 번호
  struct file* mapped_file;  // 가상주소와 매핑된 파일

  // struct list_elem mmap_elem;
  size_t swap_slot; // swap slot

  struct hash_elem hash_elem;  // hash table element
};

/*
struct mmap_file {
  int map_id;
  struct file* file;
  struct list_elem elem;
  struct list vme_list;
};
*/
void vm_init(struct hash *vm);
void vm_destroy(struct hash* vm);
bool insert_vm_entry (struct hash *vm, struct vm_entry *ve);
bool delete_vm_entry (struct hash *vm, struct vm_entry *ve);
struct vm_entry* find_vm_entry(void *vaddr);
bool load_file(void *kaddr, struct vm_entry *vme);
bool expand_stack(void* vaddr);
bool verify_stack(void* vaddr, void *esp);

#endif