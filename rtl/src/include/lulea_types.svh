`ifndef LULEA_TYPES_SVH
`define LULEA_TYPES_SVH

`include "lulea_config.svh"

package lulea_types;
    import lulea_config::*;

    // =========================================================================
    //  L1 CACHE TYPES
    // =========================================================================

    typedef logic [POP_WORD_WIDTH-1:0]    l1_pop_t;
    typedef logic [CHUNK_WORD_WIDTH-1:0]  l1_chunk_t;
    typedef logic [L1_SEQ_WORD_WIDTH-1:0] l1_seq_t;

    typedef logic [L1_WORD_ADDR_WIDTH-1:0] l1_word_addr_t;
    typedef logic [L1_SEQ_ADDR_WIDTH-1:0]  l1_seq_addr_t;

    // =========================================================================
    //  L2 CACHE TYPES
    // =========================================================================

    typedef logic [POP_WORD_WIDTH-1:0]    l2_pop_t;
    typedef logic [CHUNK_WORD_WIDTH-1:0]  l2_chunk_t;
    typedef logic [L2_SEQ_WORD_WIDTH-1:0] l2_seq_t;

    typedef logic [L2_WORD_ADDR_WIDTH-1:0] l2_word_addr_t;
    typedef logic [L2_SEQ_ADDR_WIDTH-1:0]  l2_seq_addr_t;

    // =========================================================================
    //  L3 CACHE TYPES
    // =========================================================================

    typedef logic [POP_WORD_WIDTH-1:0]    l3_pop_t;
    typedef logic [CHUNK_WORD_WIDTH-1:0]  l3_chunk_t;
    typedef logic [L3_SEQ_WORD_WIDTH-1:0] l3_seq_t;

    typedef logic [L3_WORD_ADDR_WIDTH-1:0] l3_word_addr_t;
    typedef logic [L3_SEQ_ADDR_WIDTH-1:0]  l3_seq_addr_t;
    typedef logic [L3_CHUNK_ADDR_WIDTH-1:0] l3_chunk_addr_t;
endpackage

`endif
