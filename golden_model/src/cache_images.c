#include "cache_images.h"
#include "popcount.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct
{
    const char *filename;
    void *cells;
    size_t capacity;
    unsigned width;
    size_t count;
} packed_bank_t;

typedef struct
{
    packed_bank_t pop;
    packed_bank_t sum;
    packed_bank_t seq;
} packed_level_t;

#define CACHE_BANK(images, member, filename, bits) \
    {filename, (images)->member, sizeof((images)->member) / sizeof((images)->member[0]), bits, 0}

static bool unpack_bank(FILE *file, packed_bank_t *bank)
{
    if (fseek(file, 0, SEEK_END) != 0)
    {
        return false;
    }
    const long size = ftell(file);
    const size_t capacity_bytes = (bank->capacity * bank->width + 7U) / 8U;
    if (size < 0 || (unsigned long)size > capacity_bytes || fseek(file, 0, SEEK_SET) != 0)
    {
        return false;
    }

    // All entry widths exceed the maximum seven bits of end padding.
    const size_t bytes = (size_t)size;
    bank->count = bytes * 8U / bank->width;
    if (bytes != (bank->count * bank->width + 7U) / 8U)
    {
        return false;
    }

    unsigned pending = 0;
    unsigned available = 0;
    for (size_t i = 0; i < bank->count; ++i)
    {
        uint64_t cell = 0;
        unsigned written = 0;
        while (written < bank->width)
        {
            if (available == 0)
            {
                const int byte = fgetc(file);
                if (byte == EOF)
                {
                    return false;
                }
                pending = (unsigned)byte;
                available = 8;
            }
            const unsigned remaining = bank->width - written;
            const unsigned take = available < remaining ? available : remaining;
            cell |= (uint64_t)(pending & ((1U << take) - 1U)) << written;
            written += take;
            pending >>= take;
            available -= take;
        }
        if (bank->width == CHUNK_BITS)
        {
            ((chunk_t *)bank->cells)[i] = cell;
        }
        else
        {
            ((uint32_t *)bank->cells)[i] = (uint32_t)cell;
        }
    }
    return pending == 0 && fgetc(file) == EOF && !ferror(file);
}

static bool load_bank(const char *dir, packed_bank_t *bank, char *error, size_t error_size)
{
    const size_t path_size = strlen(dir) + strlen(bank->filename) + 2U;
    char *path = malloc(path_size);
    if (path == NULL)
    {
        snprintf(error, error_size, "Out of memory opening %s", bank->filename);
        return false;
    }
    snprintf(path, path_size, "%s/%s", dir, bank->filename);
    FILE *file = fopen(path, "rb");
    if (file == NULL)
    {
        snprintf(error, error_size, "Cannot open %s", path);
        free(path);
        return false;
    }

    const bool unpacked = unpack_bank(file, bank);
    const int closed = fclose(file);
    if (!unpacked || closed != 0)
    {
        snprintf(error, error_size, "Invalid, oversized or unreadable cache bank: %s", path);
    }
    free(path);
    return unpacked && closed == 0;
}

static bool validate_level(const packed_level_t *level, uint32_t ptr_flag, size_t next_blocks)
{
    if (level->pop.count != level->sum.count)
    {
        return false;
    }
    if (level->pop.count == 0)
    {
        return level->seq.count == 0;
    }

    const chunk_t *chunks = level->pop.cells;
    const chunk_sum_t *sums = level->sum.cells;
    size_t changes = 0;
    for (size_t i = 0; i < level->pop.count; ++i)
    {
        if (sums[i] != changes)
        {
            return false;
        }
        changes += popcount_64(chunks[i]);
    }
    if (level->seq.count != changes + 1U)
    {
        return false;
    }

    const seq_cell_t *seq = level->seq.cells;
    for (size_t i = 0; i < level->seq.count; ++i)
    {
        if ((seq[i] & ptr_flag) != 0)
        {
            if ((seq[i] & (ptr_flag - 1U)) >= next_blocks)
            {
                return false;
            }
        }
        else if (seq[i] > UINT8_MAX)
        {
            return false;
        }
    }
    return true;
}

bool cache_images_load(cache_images_t *images, const char *cache_dir, char *error, size_t error_size)
{
    if (error == NULL)
    {
        error_size = 0;
    }
    if (error_size != 0)
    {
        error[0] = '\0';
    }
    if (cache_dir == NULL || cache_dir[0] == '\0')
    {
        snprintf(error, error_size, "Cache directory is required");
        return false;
    }

    packed_level_t levels[] = {
        {CACHE_BANK(images, l1_pop, "l1_pop.bin", CHUNK_BITS),
         CACHE_BANK(images, l1_chunk_sum, "l1_chunk.bin", CHUNK_SUM_BITS),
         CACHE_BANK(images, l1_seq, "l1_seq.bin", L1_SEQ_BITS)},
        {CACHE_BANK(images, l2_pop, "l2_pop.bin", CHUNK_BITS),
         CACHE_BANK(images, l2_chunk_sum, "l2_chunk.bin", CHUNK_SUM_BITS),
         CACHE_BANK(images, l2_seq, "l2_seq.bin", LOWER_SEQ_BITS)},
        {CACHE_BANK(images, l3_pop, "l3_pop.bin", CHUNK_BITS),
         CACHE_BANK(images, l3_chunk_sum, "l3_chunk.bin", CHUNK_SUM_BITS),
         CACHE_BANK(images, l3_seq, "l3_seq.bin", LOWER_SEQ_BITS)},
    };
    for (size_t i = 0; i < sizeof(levels) / sizeof(levels[0]); ++i)
    {
        if (!load_bank(cache_dir, &levels[i].pop, error, error_size) ||
            !load_bank(cache_dir, &levels[i].sum, error, error_size) ||
            !load_bank(cache_dir, &levels[i].seq, error, error_size))
        {
            return false;
        }
    }

    // Check all addresses up front so ticks can index the banks directly.
    if (levels[0].pop.count != L1_CHUNKS ||
        levels[2].pop.count % L3_CHUNKS_PER_BLOCK != 0 ||
        !validate_level(&levels[0], L1_PTR_FLAG, levels[1].pop.count) ||
        !validate_level(&levels[1], L2_PTR_FLAG, levels[2].pop.count / L3_CHUNKS_PER_BLOCK) ||
        !validate_level(&levels[2], 0, 0))
    {
        snprintf(error, error_size, "Inconsistent cache images in %s", cache_dir);
        return false;
    }
    return true;
}
