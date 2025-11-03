#include "../src/allocator.h"
#include "../unity/unity.h"
#include <unistd.h>

#define SMALL_BIN_ALLOCATION 512ul
#define SMALL_SBRK_ALLOCATION 4096ul
#define BIG_SBRK_ALLOCATION 65536ul

void setUp(void) {}

void tearDown(void) {}

static int count_bin_entries(int bin_number) {
  int count = 0;
  mchunk_t *cur = bins[bin_number];
  while (cur) {
    count++;
    cur = cur->fd_chunk;
  }
  return count;
}

void test_is_memory_released_on_top(void) {
  char *test_alloc = allocate(sizeof(char) * 32);
  free_memory(test_alloc);
  TEST_ASSERT_EQUAL(top, payload_into_mchunk(test_alloc));
}

void test_big_sbrk_allocation_freeing(void) {
  char *test_alloc = allocate(sizeof(char) * BIG_SBRK_ALLOCATION);
  char *barrier_alloc = allocate(sizeof(char) * 32);
  free_memory(test_alloc);
  TEST_ASSERT_EQUAL(bins[120], payload_into_mchunk(test_alloc));
  free_memory(barrier_alloc);
}
void test_is_memory_released_on_bin(void) {
  char *test_alloc = allocate(sizeof(char) * 32);
  char *barrier_alloc = allocate(sizeof(char) * 32);
  free_memory(test_alloc);
  TEST_ASSERT_EQUAL(payload_into_mchunk(test_alloc), bins[3]);
  free_memory(barrier_alloc);
}
void test_coalesce_two_small_chunks(void) {
  char *first_alloc = allocate(sizeof(char) * 32);
  char *second_alloc = allocate(sizeof(char) * 32);
  char *barrier_alloc = allocate(sizeof(char) * 32);
  free_memory(first_alloc);
  free_memory(second_alloc);
  TEST_ASSERT_NOT_NULL(bins[6]);
  free_memory(barrier_alloc);
}

void test_allocating_from_bins(void) {
  char *first_test_alloc = allocate(sizeof(char) * SMALL_SBRK_ALLOCATION);
  free_memory(first_test_alloc);
  char *second_test_alloc = allocate(sizeof(char) * SMALL_SBRK_ALLOCATION);
  free_memory(second_test_alloc);
  // Checking whether the second allocation was allocated from the free list
  TEST_ASSERT_EQUAL(first_test_alloc, second_test_alloc);
}

void test_coalesce_three_small_chunks(void) {
  char *alloc_1 = allocate(SMALL_BIN_ALLOCATION);
  char *alloc_2 = allocate(SMALL_BIN_ALLOCATION);
  char *alloc_3 = allocate(SMALL_BIN_ALLOCATION);
  char *barrier = allocate(SMALL_BIN_ALLOCATION);

  size_t size_1 = get_size(payload_into_mchunk(alloc_1));
  size_t size_2 = get_size(payload_into_mchunk(alloc_2));
  size_t size_3 = get_size(payload_into_mchunk(alloc_3));

  size_t coalesced_size = size_1 + size_2 + size_3;
  int coalesced_bin = find_appropriate_bin(coalesced_size);

  free_memory(alloc_1);
  free_memory(alloc_2);
  free_memory(alloc_3);

  int expected_bin_chunks_count = 1;
  int actual_bin_chunks_count = count_bin_entries(coalesced_bin);
  TEST_ASSERT_EQUAL(expected_bin_chunks_count, actual_bin_chunks_count);
  TEST_ASSERT_EQUAL(coalesced_size, get_size(bins[coalesced_bin]));
  free_memory(barrier);
}

void test_allocate_zero_bytes(void) {
  char *p = allocate(0);
  TEST_ASSERT_NOT_NULL(p);
  free_memory(p);
  TEST_ASSERT_EQUAL_PTR(top, payload_into_mchunk(p));
}

int main(void) {
  UNITY_BEGIN();
  RUN_TEST(test_is_memory_released_on_top);
  RUN_TEST(test_is_memory_released_on_bin);
  RUN_TEST(test_allocating_from_bins);
  RUN_TEST(test_big_sbrk_allocation_freeing);
  RUN_TEST(test_coalesce_two_small_chunks);
  RUN_TEST(test_coalesce_three_small_chunks);
  RUN_TEST(test_allocate_zero_bytes);
  return UNITY_END();
}
