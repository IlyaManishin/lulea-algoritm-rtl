`timescale 1ns/1ps
`include "lulea_types.svh"

module l3
    import lulea_config::*;
    import lulea_types::*;
#(
    parameter string POP_INIT_FILE = "",
    parameter string CHUNK_INIT_FILE = "",
    parameter string SEQ_INIT_FILE = ""
) (
    input  logic           clk,
    input  logic           en,
    input  l3_word_addr_t  word_addr,
    output l3_pop_t        pop_word,
    output l3_chunk_t      chunk_sum,

    input  logic           seq_en,
    input  l3_seq_addr_t   seq_addr,
    output l3_seq_t        seq_cell
);
    lulea_cache_rom #(
        .WORD_WIDTH(POP_WORD_WIDTH),
        .SIZE_BITS(L3_POP_SIZE_BITS),
        .INIT_FILE(POP_INIT_FILE)
    ) u_pop (
        .clk(clk),
        .en(en),
        .addr(word_addr),
        .dout(pop_word)
    );

    lulea_cache_rom #(
        .WORD_WIDTH(CHUNK_WORD_WIDTH),
        .SIZE_BITS(L3_CHUNK_SIZE_BITS),
        .INIT_FILE(CHUNK_INIT_FILE)
    ) u_chunk (
        .clk(clk),
        .en(en),
        .addr(l3_chunk_addr_t'(word_addr)),
        .dout(chunk_sum)
    );

    lulea_cache_rom #(
        .WORD_WIDTH(L3_SEQ_WORD_WIDTH),
        .SIZE_BITS(L3_SEQ_SIZE_BITS),
        .INIT_FILE(SEQ_INIT_FILE)
    ) u_seq (
        .clk(clk),
        .en(seq_en),
        .addr(seq_addr),
        .dout(seq_cell)
    );
endmodule
