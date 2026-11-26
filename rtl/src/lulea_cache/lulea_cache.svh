`ifndef LULEA_CACHE_SVH
`define LULEA_CACHE_SVH

`include "lulea_types.svh"

package lulea_cache_types;
    import lulea_types::*;

    // =========================================================================
    //  CACHE PORTS
    // =========================================================================

    typedef struct packed {
        l1_word_addr_t pop_addr;
        l1_word_addr_t chunk_addr;
        l1_seq_addr_t  seq_addr;
    } l1_cache_req_t;

    typedef struct packed {
        l2_word_addr_t pop_addr;
        l2_word_addr_t chunk_addr;
        l2_seq_addr_t  seq_addr;
    } l2_cache_req_t;

    typedef struct packed {
        l3_word_addr_t  pop_addr;
        l3_chunk_addr_t chunk_addr;
        l3_seq_addr_t   seq_addr;
    } l3_cache_req_t;

    typedef struct packed {
        l1_pop_t   pop_dout;
        l1_chunk_t chunk_dout;
        l1_seq_t   seq_dout;
    } l1_cache_rsp_t;

    typedef struct packed {
        l2_pop_t   pop_dout;
        l2_chunk_t chunk_dout;
        l2_seq_t   seq_dout;
    } l2_cache_rsp_t;

    typedef struct packed {
        l3_pop_t   pop_dout;
        l3_chunk_t chunk_dout;
        l3_seq_t   seq_dout;
    } l3_cache_rsp_t;
endpackage

interface lulea_cache_if;
    import lulea_cache_types::*;

    logic          en;
    l1_cache_req_t l1_req;
    l1_cache_rsp_t l1_rsp;
    l2_cache_req_t l2_req;
    l2_cache_rsp_t l2_rsp;
    l3_cache_req_t l3_req;
    l3_cache_rsp_t l3_rsp;

    modport lookup (
        output en, l1_req, l2_req, l3_req,
        input  l1_rsp, l2_rsp, l3_rsp
    );

    modport memory (
        input  en, l1_req, l2_req, l3_req,
        output l1_rsp, l2_rsp, l3_rsp
    );
endinterface

`endif
