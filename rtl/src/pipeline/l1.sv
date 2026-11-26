`timescale 1ns/1ps
`include "lulea_types.svh"

module l1
    import lulea_config::*;
    import lulea_types::*;
#(
    parameter string POP_INIT_FILE = "",
    parameter string CHUNK_INIT_FILE = "",
    parameter string SEQ_INIT_FILE = ""
) (
    input  logic        clk,
    input  logic        rst,
    input  logic        en,
    input  ipv4_parts_t ip,

    output logic        valid,
    output ipv4_parts_t ip_out,
    output logic        is_ptr,
    output l1_payload_t payload
);
    l1_word_addr_t word_addr;
    l1_pop_t       pop_word;
    l1_chunk_t     chunk_sum;

    logic          seq_en;
    l1_seq_addr_t  seq_addr;
    l1_seq_t       seq_cell;

    logic          valid_pop;
    ipv4_parts_t   ip_pop;
    pc_bit_pos_t   bit_pos;
    l1_pop_t       mask;
    popcount_t     offset;

    assign word_addr = ip.l1.chunk_idx;
    assign bit_pos = ip_pop.l1.bit_pos;
    assign mask = '1 >> (PC_BIT_POS_MAX - int'(bit_pos));
    assign seq_en = valid_pop;
    assign seq_addr = l1_seq_addr_t'(chunk_sum) + l1_seq_addr_t'(offset);

    assign {is_ptr, payload} = seq_cell;

    always_ff @(posedge clk) begin
        if (rst) begin
            valid_pop <= 1'b0;
            valid     <= 1'b0;
        end else begin
            valid_pop <= en;
            valid     <= valid_pop;
        end

        ip_pop <= ip;
        ip_out <= ip_pop;
    end

    popcount #(
        .W(POP_WORD_WIDTH)
    ) u_popcount (
        .x(pop_word & mask),
        .cnt(offset)
    );

    lulea_cache_rom #(
        .WORD_WIDTH(POP_WORD_WIDTH),
        .SIZE_BITS(L1_POP_SIZE_BITS),
        .INIT_FILE(POP_INIT_FILE)
    ) u_pop (
        .clk(clk),
        .en(en),
        .addr(word_addr),
        .dout(pop_word)
    );

    lulea_cache_rom #(
        .WORD_WIDTH(CHUNK_WORD_WIDTH),
        .SIZE_BITS(L1_CHUNK_SIZE_BITS),
        .INIT_FILE(CHUNK_INIT_FILE)
    ) u_chunk (
        .clk(clk),
        .en(en),
        .addr(word_addr),
        .dout(chunk_sum)
    );

    lulea_cache_rom #(
        .WORD_WIDTH(L1_SEQ_WORD_WIDTH),
        .SIZE_BITS(L1_SEQ_SIZE_BITS),
        .INIT_FILE(SEQ_INIT_FILE)
    ) u_seq (
        .clk(clk),
        .en(seq_en),
        .addr(seq_addr),
        .dout(seq_cell)
    );
endmodule
