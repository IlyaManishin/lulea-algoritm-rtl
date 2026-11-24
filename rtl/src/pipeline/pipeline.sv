`timescale 1ns/1ps
`include "lulea_types.svh"

module pipeline
    import lulea_types::*;
(
    input  logic           ref_clk,
    input  logic           en,

    input  l1_word_addr_t  l1_word_addr,
    input  l1_seq_addr_t   l1_seq_addr,
    output l1_pop_t        l1_pop_word,
    output l1_chunk_t      l1_chunk_sum,
    output l1_seq_t        l1_seq_cell,

    input  l2_word_addr_t  l2_word_addr,
    input  l2_seq_addr_t   l2_seq_addr,
    output l2_pop_t        l2_pop_word,
    output l2_chunk_t      l2_chunk_sum,
    output l2_seq_t        l2_seq_cell,

    input  l3_word_addr_t  l3_word_addr,
    input  l3_seq_addr_t   l3_seq_addr,
    output l3_pop_t        l3_pop_word,
    output l3_chunk_t      l3_chunk_sum,
    output l3_seq_t        l3_seq_cell
);

(* DONT_TOUCH = "TRUE" *)
    l1 #(
        .POP_INIT_FILE("l1_pop.mem"),
        .CHUNK_INIT_FILE("l1_chunk.mem"),
        .SEQ_INIT_FILE("l1_seq.mem")
    ) u_l1 (
        .clk(ref_clk),
        .en(en),
        .word_addr(l1_word_addr),
        .pop_word(l1_pop_word),
        .chunk_sum(l1_chunk_sum),
        .seq_en(en),
        .seq_addr(l1_seq_addr),
        .seq_cell(l1_seq_cell)
    );
(* DONT_TOUCH = "TRUE" *)
    l2 #(
        .POP_INIT_FILE("l2_pop.mem"),
        .CHUNK_INIT_FILE("l2_chunk.mem"),
        .SEQ_INIT_FILE("l2_seq.mem")
    ) u_l2 (
        .clk(ref_clk),
        .en(en),
        .word_addr(l2_word_addr),
        .pop_word(l2_pop_word),
        .chunk_sum(l2_chunk_sum),
        .seq_en(en),
        .seq_addr(l2_seq_addr),
        .seq_cell(l2_seq_cell)
    );
(* DONT_TOUCH = "TRUE" *)
    l3 #(
        .POP_INIT_FILE("l3_pop.mem"),
        .CHUNK_INIT_FILE("l3_chunk.mem"),
        .SEQ_INIT_FILE("l3_seq.mem")
    ) u_l3 (
        .clk(ref_clk),
        .en(en),
        .word_addr(l3_word_addr),
        .pop_word(l3_pop_word),
        .chunk_sum(l3_chunk_sum),
        .seq_en(en),
        .seq_addr(l3_seq_addr),
        .seq_cell(l3_seq_cell)
    );
endmodule
