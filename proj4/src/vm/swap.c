#include "vm/swap.h"
#include "devices/block.h"
#include "threads/vaddr.h"
#include <bitmap.h>

#define SECTORS_PER_PAGE (PGSIZE / BLOCK_SECTOR_SIZE)

static struct block *swap_block;
static struct bitmap *swap_bitmap;

void swap_init(void) {
  swap_block = block_get_role(BLOCK_SWAP);
  if (swap_block == NULL) {
    return;
  }

  size_t swap_size = block_size(swap_block) / SECTORS_PER_PAGE;
  swap_bitmap = bitmap_create(swap_size);
  if (swap_bitmap == NULL) {
    PANIC("Swap bitmap creation failed");
    return;
  }
  bitmap_set_all(swap_bitmap, false);
}

size_t swap_out(void *kaddr) {
  if (swap_block == NULL) {
    PANIC("Swap block device not available");
  }
  size_t slot = bitmap_scan_and_flip(swap_bitmap, 0, 1, false);
  if (slot == BITMAP_ERROR) {
    PANIC("SWAP FULL");
  }
  for (size_t i = 0; i < SECTORS_PER_PAGE; i++) {
    block_write(swap_block, slot * SECTORS_PER_PAGE + i,
                kaddr + i * BLOCK_SECTOR_SIZE);
  }
  return slot;
}

void swap_in(size_t slot, void *kaddr) {
  if (swap_block == NULL || !bitmap_test(swap_bitmap, slot)) {
    PANIC("Invalid swap slot");
  }
  for (int i = 0; i < SECTORS_PER_PAGE; i++) {
    block_read(swap_block, slot * SECTORS_PER_PAGE + i,
               kaddr + i * BLOCK_SECTOR_SIZE);
  }
  bitmap_set(swap_bitmap, slot, false);
}

void swap_free(size_t slot) {
  if(swap_bitmap != NULL && bitmap_test(swap_bitmap, slot)) {
    bitmap_set(swap_bitmap, slot, false);
  }
}