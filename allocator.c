#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

#include "allocator.h"

#define MIN_CHUNK_SIZE sizeof(mchunk_t)
#define CHUNK_HDR_SIZE 2 * sizeof(size_t)
#define MMAP_THRESHOLD 131072ul
#define MEM_ALIGNMENT 16ul
#define HEAP_PAGE 32768ul

// Flags
#define PREV_INUSE 0x01
#define IS_MMAP 0x02

#define SBRK_ERR (void *)-1

// This chunk is always placed on top of the accessible memory and new chunks
// are split off of it. During the allocation it may be enlarged if necessary.
static struct mchunk_t *top = NULL;

// Calculates what the minimal size of chunk must be in order to house chunks
// header and its payload.
size_t memory_size(size_t size) {
  return size + CHUNK_HDR_SIZE <= MIN_CHUNK_SIZE ? MIN_CHUNK_SIZE
                                                 : size + CHUNK_HDR_SIZE;
}
// Our memory chunks are going to be 16 bit aligned in order for our heap to
// work correctly.
size_t align_up(size_t size, size_t alignment) {
  return size % alignment == 0 ? size : size - size % alignment + alignment;
}
// Extends the top by the minimum number of pages to house a chunk of given size
void *extend_top(size_t size) {
  return sbrk((size / HEAP_PAGE) * HEAP_PAGE + HEAP_PAGE);
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
void *allocate(size_t size) {
  size_t m_size = align_up(memory_size(size), MEM_ALIGNMENT);
  if (m_size > MMAP_THRESHOLD) {
    // mmap() logic here
  } else {
    if (!top) {
      void *alloc_result = sbrk(HEAP_PAGE);
      if (alloc_result == SBRK_ERR) {
        return NULL;
      }
      top = (mchunk_t *)alloc_result;
      top->size = HEAP_PAGE;
      top->prev_size = 0;
    }
    // We can't slice off the whole top chunk because it requires having some
    // space left for its header
    if (top->size <= m_size + CHUNK_HDR_SIZE) {
      void *result = extend_top(m_size + CHUNK_HDR_SIZE);
      if (result == SBRK_ERR) {
        return NULL;
      }
    }
    void *return_ptr = (char *)top + CHUNK_HDR_SIZE;
    top->size = m_size;
    top = (mchunk_t *)((char *)top + m_size);
    top->size = (char *)sbrk(0) - (char *)top;
    top->prev_size = m_size;
    return return_ptr;
  }

  // Im going to implement the bin logic later while writing the free
  // function.
  return NULL;
}

void test_allocate(size_t alloc_size) {
  char *d = allocate(sizeof(char) * alloc_size);
  if (!d) {
    return;
  }
  d[alloc_size - 1] = 't';
  printf("My char:       %p\nTop chunk:     %p\nProgram break: %p\n%c\n", d,
         top, sbrk(0), d[alloc_size - 1]);
}

int main() {
  test_allocate(30);
  return 0;
}
