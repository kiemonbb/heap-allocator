#include <stddef.h>
#include <stdio.h>
#include <unistd.h>

#include "allocator.h"

#define MIN_CHUNK_SIZE sizeof(mchunk_t)
#define CHUNK_HDR_SIZE 2 * sizeof(size_t)
#define MMAP_THRESHOLD 131072
#define MEM_ALIGNMENT 16
#define HEAP_PAGE 4096

// Flags
#define PREV_INUSE 0x01
#define IS_MMAP 0x02

// This chunk is always placed on top of the accessible memory and new chunks
// are split off of it. During the allocation it may be enlarged if necessary.
static struct mchunk_t *top = NULL;

// Calculates what the minimal size of chunk must be in order to house chunks
// header and its payload.
inline size_t memory_size(size_t size) {
  return size + CHUNK_HDR_SIZE <= MIN_CHUNK_SIZE ? MIN_CHUNK_SIZE
                                                 : size + CHUNK_HDR_SIZE;
}
// Our memory chunks are going to be 16 bit aligned in order for our heap to
// work correctly.
inline size_t align_up(size_t size, size_t alignment) {
  return size % alignment == 0 ? size : size - size % alignment + alignment;
}

/*  For allocating memory we're gonna consider 3 possibilities
 *  1.  If requested memory is higher than MMAP_THRESHOLD, then the memory is
 *      going to be reserved using mmap() and not sliced from the top
 *  2.  Check the memory bins for free chunks that fit our request and do the
 *      necessary coalescing
 *  3.  Split off part of the top chunk and enlarge the top if necessary
 *
 *  If at some point it becomes impossible to allocate this memory the function
 *  is going to return NULL
 */
void *allocate(size_t size) { return NULL; }
