#include <stddef.h>

// Our default struct containing all the necessary information about our memory
// chunks
typedef struct mchunk_t {
  size_t prev_size;
  size_t size; // the last 2 bits here are going to be used as flags, because of
               // the 16 bit alignment
  struct mchunk_t *fd_chunk;
  struct mchunk_t *bk_chunk;
} mchunk_t;
