#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <stddef.h>

#define MIN_CHUNK_SIZE sizeof(mchunk_t)
#define CHUNK_HDR_SIZE 2 * sizeof(size_t)
#define MMAP_THRESHOLD 131072ul
#define MEM_ALIGNMENT 16ul
#define HEAP_PAGE 32768ul

// Flags
#define PREV_INUSE 0x01
#define IS_MMAP 0x02

#define SBRK_ERR (void *)-1

// Our default struct containing all the necessary information about our memory
// chunks
typedef struct mchunk_t {
  size_t prev_size;
  size_t size; // the last 2 bits here are going to be used as flags, because of
               // the 16 bit alignment
  struct mchunk_t *fd_chunk;
  struct mchunk_t *bk_chunk;
} mchunk_t;


size_t calculate_needed_memory(size_t chunks_payload);

size_t align_up_to_multiple_of_16(size_t number_to_align);

size_t calculate_aligned_memory(size_t requested_size);

void *create_top();

void *extend_top(size_t memory_size);

int is_top_too_small(size_t memory_size);

void *create_chunk_and_return_payloads_pointer(size_t memory_size);

void *allocate_with_mmap(size_t memory_size);

void *allocate_with_sbrk(size_t memory_size);

void *allocate(size_t);

#endif
