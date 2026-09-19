#pragma once

#include <stdbool.h>
#include <stdint.h>

// =========================================================================
//  BASE
// =========================================================================

typedef uint16_t port_id;

typedef struct
{
    bool is_leaf : 1;
    uint32_t ref : 31; // port or next table
} lulea_ref_t;

#define POPCOUNT_CHUNK_SIZE 64

// =========================================================================
//  FIRST ROUTING LEVEL  (16 bits: /1 -> /16)
// =========================================================================

#define L1_MASK_SIZE 16
#define L1_POPCOUNT_MAX L1_MASK_SIZE

#define L1_REFS_COUNT  ((uint64_t)1 << L1_MASK_SIZE)

// =========================================================================
//  SECOND ROUTING LEVEL (8 bits: /17 -> /24)
// =========================================================================

#define L2_LEVEL_BITS 8
#define L2_MASK_SIZE 16
#define L2_POPCOUNT_MAX L2_MASK_SIZE

#define L2_REFS_COUNT  ((uint64_t)1 << L2_MASK_SIZE)

// =========================================================================
//  THIRD ROUTING LEVEL (8 bits: /25 -> /32)
// =========================================================================

#define L3_LEVEL_BITS 8
#define L3_MASK_SIZE 16
#define L3_POPCOUNT_MAX L3_MASK_SIZE

#define L3_REFS_COUNT ((uint64_t)1 << L3_MASK_SIZE)
