`timescale 1ns/1ps
`include "lulea_cache.svh"

module l1
    import lulea_config::*;
    import lulea_types::*;
    import lulea_cache_types::*;
(
    input  logic        clk,
    input  logic        rst,
    input  logic        valid_in,
    input  ipv4_parts_t ip,

    output logic        valid,
    output ipv4_parts_t ip_out,
    output logic        is_ptr,
    output l1_payload_t payload,

    output l1_cache_req_t cache_req,
    input  l1_cache_rsp_t cache_rsp
);
    l1_word_addr_t word_addr;

    logic          valid_pop;
    ipv4_parts_t   ip_pop;
    pc_bit_pos_t   bit_pos;
    l1_pop_t       mask;
    popcount_t     offset;

    assign word_addr = ip.l1.chunk_idx;
    assign bit_pos = ip_pop.l1.bit_pos;
    assign mask = '1 >> (PC_BIT_POS_MAX - int'(bit_pos));
    assign cache_req.pop_addr = word_addr;
    assign cache_req.chunk_addr = word_addr;
    assign cache_req.seq_addr = l1_seq_addr_t'(cache_rsp.chunk_dout) + l1_seq_addr_t'(offset);

    assign {is_ptr, payload} = cache_rsp.seq_dout;

    always_ff @(posedge clk) begin
        if (rst) begin
            valid_pop <= 1'b0;
            valid     <= 1'b0;
        end else begin
            valid_pop <= valid_in;
            valid     <= valid_pop;
        end

        ip_pop <= ip;
        ip_out <= ip_pop;
    end

    popcount #(
        .W(POP_WORD_WIDTH)
    ) u_popcount (
        .x(cache_rsp.pop_dout & mask),
        .cnt(offset)
    );
endmodule
