#ifndef LULEA_CACHE_IMAGES_H
#define LULEA_CACHE_IMAGES_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Basys 3 allocation from stats/cache_stats/TOTALS.md.
enum
{
    L1_CHUNKS = 4096,
    L1_SEQ_CELLS = 16384,
    L2_CHUNKS = 8192,
    L2_SEQ_CELLS = 20480,
    L3_CHUNKS = 512,
    L3_CHUNK_SUM_CELLS = 1024,
    L3_SEQ_CELLS = 2048,
    L3_CHUNKS_PER_BLOCK = 4,

    CHUNK_BITS = 64,
    CHUNK_SUM_BITS = 18,
    L1_SEQ_BITS = 18,
    LOWER_SEQ_BITS = 9,
    L1_PTR_FLAG = 1U << (L1_SEQ_BITS - 1),
    L2_PTR_FLAG = 1U << (LOWER_SEQ_BITS - 1),
    CHUNK_BIT_MASK = CHUNK_BITS - 1,
};

typedef uint64_t chunk_t;
typedef uint32_t chunk_sum_t;
typedef uint32_t seq_cell_t;

typedef struct
{
    chunk_t l1_pop[L1_CHUNKS];
    chunk_sum_t l1_chunk_sum[L1_CHUNKS];
    seq_cell_t l1_seq[L1_SEQ_CELLS];
    chunk_t l2_pop[L2_CHUNKS];
    chunk_sum_t l2_chunk_sum[L2_CHUNKS];
    seq_cell_t l2_seq[L2_SEQ_CELLS];
    chunk_t l3_pop[L3_CHUNKS];
    chunk_sum_t l3_chunk_sum[L3_CHUNK_SUM_CELLS];
    seq_cell_t l3_seq[L3_SEQ_CELLS];
} cache_images_t;

bool cache_images_load(cache_images_t *images, const char *cache_dir, char *error, size_t error_size);

#endif
