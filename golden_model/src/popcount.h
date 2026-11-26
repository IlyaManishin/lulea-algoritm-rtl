#ifndef LULEA_MODEL_POPCOUNT_H
#define LULEA_MODEL_POPCOUNT_H

#include <stdint.h>

// Sum adjacent groups in parallel, doubling the group width at each step.
static inline uint8_t popcount_32(uint32_t value)
{
    value = (value & 0x55555555U) + ((value >> 1) & 0x55555555U);
    value = (value & 0x33333333U) + ((value >> 2) & 0x33333333U);
    value = (value & 0x0f0f0f0fU) + ((value >> 4) & 0x0f0f0f0fU);
    value = (value & 0x00ff00ffU) + ((value >> 8) & 0x00ff00ffU);
    return (uint8_t)((value & 0x0000ffffU) + (value >> 16));
}

static inline uint8_t popcount_64(uint64_t value)
{
    value = (value & UINT64_C(0x5555555555555555)) + ((value >> 1) & UINT64_C(0x5555555555555555));
    value = (value & UINT64_C(0x3333333333333333)) + ((value >> 2) & UINT64_C(0x3333333333333333));
    value = (value & UINT64_C(0x0f0f0f0f0f0f0f0f)) + ((value >> 4) & UINT64_C(0x0f0f0f0f0f0f0f0f));
    value = (value & UINT64_C(0x00ff00ff00ff00ff)) + ((value >> 8) & UINT64_C(0x00ff00ff00ff00ff));
    value = (value & UINT64_C(0x0000ffff0000ffff)) + ((value >> 16) & UINT64_C(0x0000ffff0000ffff));
    return (uint8_t)((value & UINT64_C(0xffffffff)) + (value >> 32));
}

#endif
