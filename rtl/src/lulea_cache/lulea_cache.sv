`timescale 1ns/1ps
`include "lulea_cache.svh"

module lulea_cache
    import lulea_config::*;
#(
    parameter string L1_POP_INIT_FILE = "l1_pop.mem",
    parameter string L1_CHUNK_INIT_FILE = "l1_chunk.mem",
    parameter string L1_SEQ_INIT_FILE = "l1_seq.mem",

    parameter string L2_POP_INIT_FILE = "l2_pop.mem",
    parameter string L2_CHUNK_INIT_FILE = "l2_chunk.mem",
    parameter string L2_SEQ_INIT_FILE = "l2_seq.mem",

    parameter string L3_POP_INIT_FILE = "l3_pop.mem",
    parameter string L3_CHUNK_INIT_FILE = "l3_chunk.mem",
    parameter string L3_SEQ_INIT_FILE = "l3_seq.mem"
) (
    input logic clk,
    lulea_cache_if.memory cache
);
    // =========================================================================
    //  L1 CACHE
    // =========================================================================

    logic l1_pop_en;
    logic l1_chunk_en;
    logic l1_seq_en;

    assign l1_pop_en   = cache.en & cache.l1_req.pop_en;
    assign l1_chunk_en = cache.en & cache.l1_req.chunk_en;
    assign l1_seq_en   = cache.en & cache.l1_req.seq_en;

    lulea_cache_rom #(
        .WORD_WIDTH(POP_WORD_WIDTH),
        .SIZE_BITS(L1_POP_SIZE_BITS),
        .INIT_FILE(L1_POP_INIT_FILE)
    ) u_l1_pop (
        .clk(clk),
        .en(l1_pop_en),
        .addr(cache.l1_req.pop_addr),
        .dout(cache.l1_rsp.pop_dout)
    );

    lulea_cache_rom #(
        .WORD_WIDTH(CHUNK_WORD_WIDTH),
        .SIZE_BITS(L1_CHUNK_SIZE_BITS),
        .INIT_FILE(L1_CHUNK_INIT_FILE)
    ) u_l1_chunk (
        .clk(clk),
        .en(l1_chunk_en),
        .addr(cache.l1_req.chunk_addr),
        .dout(cache.l1_rsp.chunk_dout)
    );

    lulea_cache_rom #(
        .WORD_WIDTH(L1_SEQ_WORD_WIDTH),
        .SIZE_BITS(L1_SEQ_SIZE_BITS),
        .INIT_FILE(L1_SEQ_INIT_FILE)
    ) u_l1_seq (
        .clk(clk),
        .en(l1_seq_en),
        .addr(cache.l1_req.seq_addr),
        .dout(cache.l1_rsp.seq_dout)
    );

    // =========================================================================
    //  L2 CACHE
    // =========================================================================

    logic l2_pop_en;
    logic l2_chunk_en;
    logic l2_seq_en;

    assign l2_pop_en   = cache.en & cache.l2_req.pop_en;
    assign l2_chunk_en = cache.en & cache.l2_req.chunk_en;
    assign l2_seq_en   = cache.en & cache.l2_req.seq_en;

    lulea_cache_rom #(
        .WORD_WIDTH(POP_WORD_WIDTH),
        .SIZE_BITS(L2_POP_SIZE_BITS),
        .INIT_FILE(L2_POP_INIT_FILE)
    ) u_l2_pop (
        .clk(clk),
        .en(l2_pop_en),
        .addr(cache.l2_req.pop_addr),
        .dout(cache.l2_rsp.pop_dout)
    );

    lulea_cache_rom #(
        .WORD_WIDTH(CHUNK_WORD_WIDTH),
        .SIZE_BITS(L2_CHUNK_SIZE_BITS),
        .INIT_FILE(L2_CHUNK_INIT_FILE)
    ) u_l2_chunk (
        .clk(clk),
        .en(l2_chunk_en),
        .addr(cache.l2_req.chunk_addr),
        .dout(cache.l2_rsp.chunk_dout)
    );

    lulea_cache_rom #(
        .WORD_WIDTH(L2_SEQ_WORD_WIDTH),
        .SIZE_BITS(L2_SEQ_SIZE_BITS),
        .INIT_FILE(L2_SEQ_INIT_FILE)
    ) u_l2_seq (
        .clk(clk),
        .en(l2_seq_en),
        .addr(cache.l2_req.seq_addr),
        .dout(cache.l2_rsp.seq_dout)
    );

    // =========================================================================
    //  L3 CACHE
    // =========================================================================

    logic l3_pop_en;
    logic l3_chunk_en;
    logic l3_seq_en;

    assign l3_pop_en   = cache.en & cache.l3_req.pop_en;
    assign l3_chunk_en = cache.en & cache.l3_req.chunk_en;
    assign l3_seq_en   = cache.en & cache.l3_req.seq_en;

    lulea_cache_rom #(
        .WORD_WIDTH(POP_WORD_WIDTH),
        .SIZE_BITS(L3_POP_SIZE_BITS),
        .INIT_FILE(L3_POP_INIT_FILE)
    ) u_l3_pop (
        .clk(clk),
        .en(l3_pop_en),
        .addr(cache.l3_req.pop_addr),
        .dout(cache.l3_rsp.pop_dout)
    );

    lulea_cache_rom #(
        .WORD_WIDTH(CHUNK_WORD_WIDTH),
        .SIZE_BITS(L3_CHUNK_SIZE_BITS),
        .INIT_FILE(L3_CHUNK_INIT_FILE)
    ) u_l3_chunk (
        .clk(clk),
        .en(l3_chunk_en),
        .addr(cache.l3_req.chunk_addr),
        .dout(cache.l3_rsp.chunk_dout)
    );

    lulea_cache_rom #(
        .WORD_WIDTH(L3_SEQ_WORD_WIDTH),
        .SIZE_BITS(L3_SEQ_SIZE_BITS),
        .INIT_FILE(L3_SEQ_INIT_FILE)
    ) u_l3_seq (
        .clk(clk),
        .en(l3_seq_en),
        .addr(cache.l3_req.seq_addr),
        .dout(cache.l3_rsp.seq_dout)
    );
endmodule
