#include "popcount.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned count_bits(uint64_t value)
{
    unsigned count = 0;
    for (; value != 0; value >>= 1)
    {
        count += (unsigned)(value & 1U);
    }
    return count;
}

static bool check_counts(uint64_t value)
{
    const uint32_t lower = (uint32_t)value;
    return popcount_64(value) == count_bits(value) && popcount_32(lower) == count_bits(lower);
}

int main(void)
{
    if (!check_counts(0) || !check_counts(UINT64_MAX))
    {
        fputs("Incorrect popcount for all-zero or all-one input\n", stderr);
        return EXIT_FAILURE;
    }
    for (unsigned bit = 0; bit < 64; ++bit)
    {
        if (!check_counts(UINT64_C(1) << bit))
        {
            fprintf(stderr, "Incorrect popcount for bit %u\n", bit);
            return EXIT_FAILURE;
        }
    }

    uint64_t value = UINT64_C(0x395abfeee21744c9);
    for (unsigned i = 0; i < 65536; ++i)
    {
        value ^= value << 13;
        value ^= value >> 7;
        value ^= value << 17;
        if (!check_counts(value))
        {
            fprintf(stderr, "Incorrect popcount for sample %u\n", i);
            return EXIT_FAILURE;
        }
    }
    return EXIT_SUCCESS;
}
