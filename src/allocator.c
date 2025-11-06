#include <math.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include "allocator.h"

mchunk_t *top = NULL;
mchunk_t *bins[BIN_COUNT] = {NULL};

void *create_top() {
  void *sbrk_result = sbrk(HEAP_PAGE);
  if (sbrk_result == SBRK_ERR) {
    return sbrk_result;
  }
  top = (mchunk_t *)sbrk_result;
  top->size_with_flags = HEAP_PAGE;
  set_chunks_flag(top, PREV_INUSE);
  top->prev_size = 0;
  return sbrk_result;
}

// pimpcio
//  Extends the top by the minimum number of pages to house a chunk of given
//  size
void *extend_top(size_t memory_size) {
  size_t minimal_extension_size =
      (memory_size + HEAP_PAGE - 1) / (HEAP_PAGE)*HEAP_PAGE;
  void *extension_result = sbrk(minimal_extension_size);
  if (extension_result == SBRK_ERR) {
    return extension_result;
  }
  top->size_with_flags += minimal_extension_size;
  return extension_result;
}

// We can't slice off the whole top chunk because it requires having some
// space left for its header
int is_top_too_small(size_t memory_size) {
  size_t top_size = get_size(top);
  if (top_size < memory_size) {
    return 1;
  }
  return (top_size - memory_size) < MIN_CHUNK_SIZE;
}

/* Two LSBs of our mchunks size are flags containing whether:
 * 1. The previous chunk is currently in use
 * 2. Is this chunk in use
 * 3. This chunk was allocated using mmap() instead of sbrk()
 */

int is_prev_mchunk_in_use(mchunk_t *memory_chunk) {
  return memory_chunk->size_with_flags & PREV_INUSE;
}

int is_chunk_mmaped(mchunk_t *memory_chunk) {
  return memory_chunk->size_with_flags & IS_MMAP;
}

int is_in_use(mchunk_t *memory_chunk) {
  return memory_chunk->size_with_flags & IS_INUSE;
}

void set_chunks_flag(mchunk_t *memory_chunk, unsigned long flag) {
  memory_chunk->size_with_flags |= flag;
}

void unset_chunks_flag(mchunk_t *memory_chunk, unsigned long flag) {
  memory_chunk->size_with_flags &= ~(flag);
}

size_t get_size(mchunk_t *memory_chunk) {
  return memory_chunk->size_with_flags & ~(ALL_FLAGS);
}

size_t calculate_needed_memory(size_t chunks_payload) {
  size_t needed_memory = chunks_payload + CHUNK_HDR_SIZE <= MIN_CHUNK_SIZE
                             ? MIN_CHUNK_SIZE
                             : chunks_payload + CHUNK_HDR_SIZE;
  return needed_memory;
}

// Our memory chunks are going to be 16 bit aligned in order for our heap to
// work correctly on 64 bit systems.
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

  // set allocated chunks size and flag
  top->size_with_flags = memory_size;
  set_chunks_flag(top, IS_INUSE);

  // create 'new' top and set its variables
  top = (mchunk_t *)((char *)top + memory_size);
  top->size_with_flags = (char *)sbrk(0) - (char *)top;
  top->prev_size = memory_size;
  set_chunks_flag(top, PREV_INUSE);

  return return_ptr;
}
void *mchunk_into_payload(mchunk_t *memory_chunk) {
  void *payload_ptr = (void *)((char *)(memory_chunk) + CHUNK_HDR_SIZE);
  return payload_ptr;
}
mchunk_t *payload_into_mchunk(void *payload_ptr) {
  mchunk_t *memory_chunk = (mchunk_t *)((char *)payload_ptr - CHUNK_HDR_SIZE);
  return memory_chunk;
}

mchunk_t *get_next_chunk(mchunk_t *memory_chunk) {
  mchunk_t *next_chunk =
      (mchunk_t *)((char *)memory_chunk + get_size(memory_chunk));
  return next_chunk;
}
mchunk_t *get_previous_chunk(mchunk_t *memory_chunk) {
  mchunk_t *previous_chunk =
      (mchunk_t *)((char *)memory_chunk - memory_chunk->prev_size);
  return previous_chunk;
}

void remove_from_bin(mchunk_t *memory_chunk) {

  // check whether memory_chunk is some bins head
  for (int i = 0; i < BIN_COUNT; ++i) {
    if (bins[i] == memory_chunk) {
      bins[i] = memory_chunk->fd_chunk;
      if (memory_chunk->fd_chunk) {
        memory_chunk->fd_chunk->bk_chunk = NULL;
      }
      memory_chunk->fd_chunk = memory_chunk->bk_chunk = NULL;
      return;
    }
  }

  if (memory_chunk->bk_chunk) {
    memory_chunk->bk_chunk->fd_chunk = memory_chunk->fd_chunk;
  }
  if (memory_chunk->fd_chunk) {
    memory_chunk->fd_chunk->bk_chunk = memory_chunk->bk_chunk;
  }
  memory_chunk->fd_chunk = memory_chunk->bk_chunk = NULL;
}

mchunk_t *coalesce_two_chunks(mchunk_t *previous_chunk, mchunk_t *next_chunk) {
  previous_chunk->size_with_flags += get_size(next_chunk);
  return previous_chunk;
}

// TODO: refactor this function
//       correctly set prev_size variable and prev_in_use flag for new chunk
mchunk_t *coalesce_neighbouring_chunks(mchunk_t *memory_chunk) {
  // If exists coalesce with previous chunk
  if (!(memory_chunk->prev_size == 0) && !is_prev_mchunk_in_use(memory_chunk)) {
    mchunk_t *previous_chunk = get_previous_chunk(memory_chunk);
    remove_from_bin(previous_chunk);
    memory_chunk = coalesce_two_chunks(previous_chunk, memory_chunk);
  }

  // If exists and is not top coalesce with next chunk
  mchunk_t *next_chunk = get_next_chunk(memory_chunk);
  if (next_chunk != top && !is_in_use(next_chunk)) {
    remove_from_bin(next_chunk);
    memory_chunk = coalesce_two_chunks(memory_chunk, next_chunk);
  }

  mchunk_t *following = get_next_chunk(memory_chunk);
  following->prev_size = get_size(memory_chunk);

  return memory_chunk;
}

/*
 * bins[0] = N/A
 * bins[1] = unsorted bin (to be implemented)
 * bins[2-63] = small bins(ranging from 32 to 1008 bytes)
 * bins[64-95] = large bins with 64 byte spacing
 * bins[96-111] = large bins with 512 byte spacing
 * bins[112-119] = large bins with 4096 byte spacing
 * bins[120-121] = large bins with 32768 byte spacing
 * bins[122] = whats left
 * */
int find_appropriate_bin(size_t memory_size) {
  // SMALL BINS
  // TODO: get rid of magic numbers
  if (memory_size <= SMALL_BIN_MAX) {
    return memory_size / 16;
  } else if (memory_size <= LARGE_BIN_64_BYTE_SPACING_MAX) {
    return 63 + ceil((memory_size - SMALL_BIN_MAX) / 64.0);

  } else if (memory_size <= LARGE_BIN_512_BYTE_SPACING_MAX) {
    return 95 + ceil((memory_size - LARGE_BIN_64_BYTE_SPACING_MAX) / 512.0);

  } else if (memory_size <= LARGE_BIN_4096_BYTE_SPACING_MAX) {
    return 111 + ceil((memory_size - LARGE_BIN_512_BYTE_SPACING_MAX) / 4096.0);

  } else if (memory_size <= LARGE_BIN_32768_BYTE_SPACING_MAX) {
    return 119 +
           ceil((memory_size - LARGE_BIN_4096_BYTE_SPACING_MAX) / 32768.0);

  } else {
    return BIN_COUNT - 1;
  }
}

void add_chunk_to_bin(mchunk_t *memory_chunk) {
  size_t true_size = get_size(memory_chunk);
  int bin_number = find_appropriate_bin(true_size);
  memory_chunk->fd_chunk = memory_chunk->bk_chunk = NULL;

  if (bins[bin_number] == NULL) {
    bins[bin_number] = memory_chunk;
    return;
  }

  if (get_size(memory_chunk) <= get_size(bins[bin_number])) {
    bins[bin_number]->bk_chunk = memory_chunk;
    memory_chunk->fd_chunk = bins[bin_number];
    bins[bin_number] = memory_chunk;
    return;
  }

  mchunk_t *current = bins[bin_number];
  while (current->fd_chunk &&
         get_size(current->fd_chunk) < get_size(memory_chunk)) {
    current = current->fd_chunk;
  }

  memory_chunk->fd_chunk = current->fd_chunk;
  memory_chunk->bk_chunk = current;
  if (current->fd_chunk) {
    current->fd_chunk->bk_chunk = memory_chunk;
  }
  current->fd_chunk = memory_chunk;
}

mchunk_t *find_and_remove_chunk_from_bin(size_t memory_size) {
  int bin_number = find_appropriate_bin(memory_size);
  for (int i = bin_number; i < BIN_COUNT; i++) {
    mchunk_t *current = bins[i];
    while (current) {
      if (get_size(current) >= memory_size) {
        remove_from_bin(current);
        return current;
      }
      current = current->fd_chunk;
    }
  }
  return NULL;
}
void merge_chunk_with_top(mchunk_t *memory_chunk) {
  size_t top_size = get_size(top);
  top = memory_chunk;
  top->size_with_flags += top_size;
}

void free_sbrk_memory(mchunk_t *memory_chunk) {
  mchunk_t *coalesced_chunk = coalesce_neighbouring_chunks(memory_chunk);

  mchunk_t *next_chunk = get_next_chunk(coalesced_chunk);
  unset_chunks_flag(next_chunk, PREV_INUSE);

  // Merge newly coalesced chunk with the top
  if (next_chunk == top) {
    merge_chunk_with_top(coalesced_chunk);
    return;
  }
  add_chunk_to_bin(coalesced_chunk);
}

void *allocate_with_mmap(size_t memory_size) {}

void *allocate_with_sbrk(size_t memory_size) {
  void *memory_ptr;
  // Check bins for appropriate allocations
  mchunk_t *memory_chunk = find_and_remove_chunk_from_bin(memory_size);
  //
  // Set appropriate flags for the found chunk and its neighbour
  if (memory_chunk) {
    set_chunks_flag(memory_chunk, IS_INUSE);
    mchunk_t *following = get_next_chunk(memory_chunk);
    set_chunks_flag(following, PREV_INUSE);
    memory_ptr = mchunk_into_payload(memory_chunk);
    return memory_ptr;
  }

  // Check whether top exists and if it's big enough
  void *result_ptr = NULL;
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

  // Slice a chunk off top
  memory_ptr = create_chunk_and_return_payloads_pointer(memory_size);
  return memory_ptr;
}

void free_memory(void *payload_ptr) {
  if (!payload_ptr)
    return;
  mchunk_t *memory_chunk = payload_into_mchunk(payload_ptr);
  if (is_chunk_mmaped(memory_chunk)) {
    // UNMMAP
  } else {
    free_sbrk_memory(memory_chunk);
  }
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
