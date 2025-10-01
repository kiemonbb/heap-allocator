#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

#include "allocator.h"

// This chunk is always placed on top of the accessible memory and new chunks
// are split off of it. During the allocation it may be enlarged if necessary.
static struct mchunk_t *top = NULL;

size_t calculate_needed_memory(size_t chunks_payload) {
  size_t needed_memory = chunks_payload + CHUNK_HDR_SIZE <= MIN_CHUNK_SIZE
                             ? MIN_CHUNK_SIZE
                             : chunks_payload + CHUNK_HDR_SIZE;
  return needed_memory;
}

// Our memory chunks are going to be 16 bit aligned in order for our heap to
// work correctly.
size_t align_up_to_multiple_of_16(size_t number_to_align) {
  size_t aligned_number =
      number_to_align % MEM_ALIGNMENT == 0
          ? number_to_align
          : number_to_align - number_to_align % MEM_ALIGNMENT + MEM_ALIGNMENT;
  return aligned_number;
}

size_t calculate_aligned_memory(size_t memory_size) {
  size_t needed_memory = calculate_needed_memory(memory_size);
  size_t aligned_memory = align_up_to_multiple_of_16(needed_memory);
  return aligned_memory;
}

void *create_top() {
  void *sbrk_result = sbrk(HEAP_PAGE);
  if (sbrk_result == SBRK_ERR) {
    return sbrk_result;
  }
  top = (mchunk_t *)sbrk_result;
  top->size = HEAP_PAGE;
  top->prev_size = 0;
  return sbrk_result;
}

// Extends the top by the minimum number of pages to house a chunk of given size
void *extend_top(size_t memory_size) {
  void *extension_result =
      sbrk((memory_size / HEAP_PAGE) * HEAP_PAGE + HEAP_PAGE);
  return extension_result;
}

// We can't slice off the whole top chunk because it requires having some
// space left for its header
int is_top_too_small(size_t memory_size) {
  return top->size <= memory_size + CHUNK_HDR_SIZE ? 1 : 0;
}

/*  For allocating memory we're gonna consider 3 possibilities
 *  1.  If requested memory is higher than MMAP_THRESHOLD, then the memory
 * is going to be reserved using mmap() and not sliced from the top
 *  2.  Check the memory bins for free chunks that fit our request and do
 * the necessary coalescing
 *  3.  Split off part of the top chunk and enlarge the top if necessary
 *
 *  If at some point it becomes impossible to allocate this memory the
 * function is going to return NULL
 */

void *create_chunk_and_return_payloads_pointer(size_t memory_size) {
  void *return_ptr = (char *)top + CHUNK_HDR_SIZE;
  top->size = memory_size;
  top = (mchunk_t *)((char *)top + memory_size);
  top->size = (char *)sbrk(0) - (char *)top;
  top->prev_size = memory_size;
  return return_ptr;
}

void *allocate_with_mmap(size_t memory_size) {}

void *allocate_with_sbrk(size_t memory_size) {
  if (!top) {
    void *creation_result = create_top();
    if (creation_result == SBRK_ERR) {
      return NULL;
    }
  }
  if (is_top_too_small(memory_size)) {
    void *extension_result = extend_top(memory_size);
    if (extension_result == SBRK_ERR) {
      return NULL;
    }
  }
  void *memory_ptr = create_chunk_and_return_payloads_pointer(memory_size);
  return memory_ptr;
}

void *allocate(size_t size) {
  void *result_ptr;
  size_t memory_size = calculate_aligned_memory(size);
  if (memory_size > MMAP_THRESHOLD) {
    result_ptr = allocate_with_mmap(memory_size);
  } else {
    result_ptr = allocate_with_sbrk(memory_size);
  }
  return result_ptr;
}

int main() {
  return 0;
}
