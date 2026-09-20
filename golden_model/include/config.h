#pragma once

#include <stdbool.h>
#include <stdint.h>

// =========================================================================
//  BASE
// =========================================================================

typedef uint16_t port_id_t;
typedef uint16_t table_ref_t;

typedef struct {
  uint32_t is_leaf : 1; // 1 = port_id; 0 = next level table index
  uint32_t ref : 31;    // port or next table index
} lulea_ref_t;

typedef uint16_t popcount_t;

// =========================================================================
//  FIRST ROUTING LEVEL  (16 bits: /1 -> /16)
// =========================================================================

#define L1_LEVEL_BITS   16
#define L1_CHUNK_SIZE   64
#define L1_MASK_SIZE    L1_CHUNK_SIZE
#define L1_POPCOUNT_MAX L1_MASK_SIZE

#define L1_REFS_COUNT ((uint64_t)1 << L1_LEVEL_BITS) // 65536 elements (1024 chunks * 64 bits)

// =========================================================================
//  SECOND ROUTING LEVEL (8 bits: /17 -> /24)
// =========================================================================

#define L2_LEVEL_BITS   8
#define L2_CHUNK_SIZE   16
#define L2_MASK_SIZE    L2_CHUNK_SIZE
#define L2_POPCOUNT_MAX L2_MASK_SIZE

#define L2_REFS_COUNT ((uint64_t)1 << L2_LEVEL_BITS) // 256 elements (16 chunks * 16 bits)

// =========================================================================
//  THIRD ROUTING LEVEL  (8 bits: /25 -> /32)
// =========================================================================

#define L3_LEVEL_BITS   8
#define L3_CHUNK_SIZE   16
#define L3_MASK_SIZE    L3_CHUNK_SIZE
#define L3_POPCOUNT_MAX L3_MASK_SIZE

#define L3_REFS_COUNT ((uint64_t)1 << L3_LEVEL_BITS) // 256 elements (16 chunks * 16 bits)