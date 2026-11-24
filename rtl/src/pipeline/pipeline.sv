`timescale 1ns/1ps
`include "lulea_config.svh"

module pipeline
    import lulea_config::*;
(
    input  logic                         ref_clk,
    input  logic                         en,

    input  logic [L1_WORD_ADDR_WIDTH-1:0] l1_word_addr,
    input  logic [L1_SEQ_ADDR_WIDTH-1:0]  l1_seq_addr,
    output logic [POP_WORD_WIDTH-1:0]     l1_pop_word,
    output logic [CHUNK_WORD_WIDTH-1:0]   l1_chunk_sum,
    output logic [L1_SEQ_WORD_WIDTH-1:0]  l1_seq_cell,

    input  logic [L2_WORD_ADDR_WIDTH-1:0] l2_word_addr,
    input  logic [L2_SEQ_ADDR_WIDTH-1:0]  l2_seq_addr,
    output logic [POP_WORD_WIDTH-1:0]     l2_pop_word,
    output logic [CHUNK_WORD_WIDTH-1:0]   l2_chunk_sum,
    output logic [L2_SEQ_WORD_WIDTH-1:0]  l2_seq_cell,

    input  logic [L3_WORD_ADDR_WIDTH-1:0] l3_word_addr,
    input  logic [L3_SEQ_ADDR_WIDTH-1:0]  l3_seq_addr,
    output logic [POP_WORD_WIDTH-1:0]     l3_pop_word,
    output logic [CHUNK_WORD_WIDTH-1:0]   l3_chunk_sum,
    output logic [L3_SEQ_WORD_WIDTH-1:0]  l3_seq_cell
);

(* DONT_TOUCH = "TRUE" *)
    l1 #(
        .POP_INIT_FILE("l1_pop.mem"),
        .CHUNK_INIT_FILE("l1_chunk.mem"),
        .SEQ_INIT_FILE("l1_seq.mem")
    ) u_l1 (
        .clk(ref_clk),
        .word_en(en),
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
        .word_en(en),
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
        .word_en(en),
        .word_addr(l3_word_addr),
        .pop_word(l3_pop_word),
        .chunk_sum(l3_chunk_sum),
        .seq_en(en),
        .seq_addr(l3_seq_addr),
        .seq_cell(l3_seq_cell)
    );
endmodule
