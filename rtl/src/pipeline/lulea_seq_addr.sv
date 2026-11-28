`timescale 1ns/1ps

`include "lulea_config.svh"
`include "lulea_types.svh"

module lulea_seq_addr
    import lulea_config::*;
    import lulea_types::*;
#(
    parameter int unsigned SEQ_ADDR_WIDTH = CHUNK_WORD_WIDTH
) (
    input  logic                        clk,
    input  logic                        rst,
    input  logic                        valid_in,
    input  ipv4_parts_t                 ip,
    input  pc_bit_pos_t                 bit_pos,
    input  logic [POP_WORD_WIDTH-1:0]   pop_word,
    input  logic [CHUNK_WORD_WIDTH-1:0] chunk_sum,

    output logic                        valid,
    output ipv4_parts_t                 ip_out,
    output logic [SEQ_ADDR_WIDTH-1:0]   seq_addr
);
    typedef logic [SEQ_ADDR_WIDTH-1:0] seq_addr_t;

    popcount_half_t              offset_left, offset_left_r;
    popcount_half_t              offset_right, offset_right_r;
    popcount_t                   offset;
    logic [CHUNK_WORD_WIDTH-1:0] chunk_sum_s1, chunk_sum_r;
    logic                        valid_s1, valid_r;
    ipv4_parts_t                 ip_s1, ip_r;

    logic [POP_WORD_WIDTH-1:0] mask;
    logic [POP_WORD_WIDTH-1:0] masked_word;

    // =========================================================================
    //  POPCOUNT
    // =========================================================================

    assign mask = '1 >> (PC_BIT_POS_MAX - int'(bit_pos));

    popcount #(
        .W(POP_WORD_HALF_WIDTH)
    ) u_popcount_left (
        .x(masked_word[POP_WORD_WIDTH-1:POP_WORD_HALF_WIDTH]),
        .cnt(offset_left)
    );

    popcount #(
        .W(POP_WORD_HALF_WIDTH)
    ) u_popcount_right (
        .x(masked_word[POP_WORD_HALF_WIDTH-1:0]),
        .cnt(offset_right)
    );

    // =========================================================================
    //  PIPELINE AND SEQ ADDRESS
    // =========================================================================

    always_ff @(posedge clk) begin
        if (rst) begin
            valid_s1 <= 1'b0;
            valid_r  <= 1'b0;
        end else begin
            valid_s1 <= valid_in;
            valid_r  <= valid_s1;
        end

        masked_word  <= pop_word & mask;
        ip_s1        <= ip;
        chunk_sum_s1 <= chunk_sum;

        ip_r           <= ip_s1;
        offset_left_r  <= offset_left;
        offset_right_r <= offset_right;
        chunk_sum_r    <= chunk_sum_s1;
    end

    assign offset = popcount_t'(offset_left_r) + popcount_t'(offset_right_r);
    assign seq_addr = seq_addr_t'(chunk_sum_r) + seq_addr_t'(offset);
    assign valid = valid_r;
    assign ip_out = ip_r;
endmodule
