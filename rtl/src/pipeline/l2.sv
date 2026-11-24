`timescale 1ns/1ps
`include "lulea_config.svh"

module l2
    import lulea_config::*;
#(
    parameter string POP_INIT_FILE = "",
    parameter string CHUNK_INIT_FILE = "",
    parameter string SEQ_INIT_FILE = ""
) (
    input  logic                          clk,
    input  logic                          word_en,
    input  logic [L2_WORD_ADDR_WIDTH-1:0] word_addr,
    output logic [POP_WORD_WIDTH-1:0]     pop_word,
    output logic [CHUNK_WORD_WIDTH-1:0]   chunk_sum,

    input  logic                          seq_en,
    input  logic [L2_SEQ_ADDR_WIDTH-1:0]  seq_addr,
    output logic [L2_SEQ_WORD_WIDTH-1:0]  seq_cell
);
    lulea_cache_rom #(
        .WORD_WIDTH(POP_WORD_WIDTH),
        .SIZE_BITS(L2_POP_SIZE_BITS),
        .INIT_FILE(POP_INIT_FILE)
    ) u_pop (
        .clk(clk),
        .en(word_en),
        .addr(word_addr),
        .dout(pop_word)
    );

    lulea_cache_rom #(
        .WORD_WIDTH(CHUNK_WORD_WIDTH),
        .SIZE_BITS(L2_CHUNK_SIZE_BITS),
        .INIT_FILE(CHUNK_INIT_FILE)
    ) u_chunk (
        .clk(clk),
        .en(word_en),
        .addr(word_addr),
        .dout(chunk_sum)
    );

    // Valid addresses end at L2_SEQ_DEPTH - 1 within the 15-bit bus.
    lulea_cache_rom #(
        .WORD_WIDTH(L2_SEQ_WORD_WIDTH),
        .SIZE_BITS(L2_SEQ_SIZE_BITS),
        .INIT_FILE(SEQ_INIT_FILE)
    ) u_seq (
        .clk(clk),
        .en(seq_en),
        .addr(seq_addr),
        .dout(seq_cell)
    );
endmodule
