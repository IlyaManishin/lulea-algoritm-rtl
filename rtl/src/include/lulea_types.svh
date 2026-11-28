`ifndef LULEA_TYPES_SVH
`define LULEA_TYPES_SVH

`include "lulea_config.svh"

package lulea_types;
    import lulea_config::*;

    // =========================================================================
    //  COMMON DATAPATH TYPES
    // =========================================================================

    typedef logic [IP_WIDTH-1:0]          ipv4_t;
    typedef logic [PORT_WIDTH-1:0]        port_t;
    typedef logic [PC_BIT_POS_WIDTH-1:0]  pc_bit_pos_t;
    typedef logic [POPCOUNT_WIDTH-1:0]    popcount_t;
    typedef logic [POPCOUNT_WIDTH-2:0]    popcount_half_t;

    typedef logic [POP_WORD_WIDTH-1:0]    popcount_mask_t;

    // =========================================================================
    //  IPv4 ADDRESS FIELDS
    // =========================================================================

    typedef struct packed {
        logic [L1_IP_CHUNK_IDX_WIDTH-1:0] chunk_idx;
        pc_bit_pos_t                      bit_pos;
    } l1_ip_t;

    typedef struct packed {
        logic [L2_IP_BIT_POS_WIDTH-1:0] bit_pos;
    } l2_ip_t;

    typedef struct packed {
        logic [L3_IP_CHUNK_IDX_WIDTH-1:0] chunk_idx;
        pc_bit_pos_t                      bit_pos;
    } l3_ip_t;

    typedef struct packed {
        l1_ip_t l1;
        l2_ip_t l2;
        l3_ip_t l3;
    } ipv4_parts_t;

    // =========================================================================
    //  L1 CACHE TYPES
    // =========================================================================

    typedef logic [POP_WORD_WIDTH-1:0]    l1_pop_t;
    typedef logic [CHUNK_WORD_WIDTH-1:0]  l1_chunk_t;
    typedef logic [L1_SEQ_WORD_WIDTH-1:0] l1_seq_t;
    typedef logic [L1_PAYLOAD_WIDTH-1:0]  l1_payload_t;

    typedef logic [L1_WORD_ADDR_WIDTH-1:0] l1_word_addr_t;
    typedef logic [L1_SEQ_ADDR_WIDTH-1:0]  l1_seq_addr_t;

    // =========================================================================
    //  L2 CACHE TYPES
    // =========================================================================

    typedef logic [POP_WORD_WIDTH-1:0]    l2_pop_t;
    typedef logic [CHUNK_WORD_WIDTH-1:0]  l2_chunk_t;
    typedef logic [L2_SEQ_WORD_WIDTH-1:0] l2_seq_t;
    typedef logic [L2_PAYLOAD_WIDTH-1:0]  l2_payload_t;

    typedef logic [L2_WORD_ADDR_WIDTH-1:0] l2_word_addr_t;
    typedef logic [L2_SEQ_ADDR_WIDTH-1:0]  l2_seq_addr_t;

    // =========================================================================
    //  L3 CACHE TYPES
    // =========================================================================

    typedef logic [POP_WORD_WIDTH-1:0]    l3_pop_t;
    typedef logic [CHUNK_WORD_WIDTH-1:0]  l3_chunk_t;
    typedef logic [L3_SEQ_WORD_WIDTH-1:0] l3_seq_t;

    typedef logic [L3_WORD_ADDR_WIDTH-1:0]  l3_word_addr_t;
    typedef logic [L3_SEQ_ADDR_WIDTH-1:0]   l3_seq_addr_t;
    typedef logic [L3_CHUNK_ADDR_WIDTH-1:0] l3_chunk_addr_t;
    typedef logic [L3_PC_IDX_WIDTH-1:0]     l3_pc_idx_t;
endpackage

`endif
