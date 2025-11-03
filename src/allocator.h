#ifndef ALLOCATOR_H
#define ALLOCATOR_H

#include <math.h>
#include <stddef.h>

// TODO: turn it into function considering structs alignment
#define MIN_CHUNK_SIZE sizeof(mchunk_t)

#define CHUNK_HDR_SIZE 2 * sizeof(size_t)
#define MMAP_THRESHOLD 131072u
#define MEM_ALIGNMENT 16u
#define HEAP_PAGE 32768u

// Flags
#define PREV_INUSE 0b1
#define IS_MMAP 0b10
#define IS_INUSE 0b100
#define ALL_FLAGS 0b111

#define SBRK_ERR (void *)-1

#define BIN_COUNT 123
#define SMALL_BIN_MAX 1008
#define LARGE_BIN_64_BYTE_SPACING_MAX 3056
#define LARGE_BIN_512_BYTE_SPACING_MAX 11248
#define LARGE_BIN_4096_BYTE_SPACING_MAX 44016
#define LARGE_BIN_32768_BYTE_SPACING_MAX 142320

// Our default struct containing all the necessary information about our memory
// chunks
typedef struct mchunk_t {
  size_t prev_size;
  size_t size_with_flags; // the last 2 bits here are going to be used as flags, because of
               // the 16 bit alignment
  struct mchunk_t *fd_chunk;
  struct mchunk_t *bk_chunk;
} mchunk_t;

// This chunk is always placed on top of the accessible memory and new chunks
// are split off of it. During the allocation it may be enlarged if necessary.
extern mchunk_t *top;

extern mchunk_t *bins[BIN_COUNT];

int is_prev_mchunk_inuse(mchunk_t *memory_chunk);

int is_chunk_mmaped(mchunk_t *memory_chunk);

void set_chunks_flag(mchunk_t *memory_chunk, unsigned long flag);

void unset_chunks_flag(mchunk_t *memory_chunk, unsigned long flag);

size_t get_size(mchunk_t *memory_chunk);

void *create_top();

void *extend_top(size_t memory_size);

int is_top_too_small(size_t memory_size);

size_t calculate_needed_memory(size_t chunks_payload);

size_t align_up_to_multiple_of_16(size_t number_to_align);

size_t calculate_aligned_memory(size_t requested_size);

void *create_chunk_and_return_payloads_pointer(size_t memory_size);

void *allocate_with_mmap(size_t memory_size);

void *allocate_with_sbrk(size_t memory_size);

mchunk_t *payload_into_mchunk(void *payload_ptr);

void *mchunk_into_payload(mchunk_t *memory_chunk);

mchunk_t *get_next_chunk(mchunk_t *memory_chunk);

void add_chunk_to_freelist(mchunk_t *memory_chunk);

void create_freelist(mchunk_t *memory_chunk);

void free_memory(void *payload_ptr);

mchunk_t *get_next_chunk(mchunk_t *memory_chunk);

mchunk_t *get_previous_chunk(mchunk_t *memory_chunk);

void delete_from_bin(mchunk_t *memory_chunk);

mchunk_t *coalesce_two_chunks(mchunk_t *first_chunk, mchunk_t *second_chunk);

mchunk_t *coalesce_neighbouring_chunks(mchunk_t *memory_chunk);

int find_appropriate_bin(size_t memory_size);

void add_chunk_to_bin(mchunk_t *memory_chunk, int bin_number);

mchunk_t *find_chunk_in_bin(size_t memory_size);

void free_sbrk_memory(mchunk_t *memory_chunk);

void *allocate(size_t size);

#endif
