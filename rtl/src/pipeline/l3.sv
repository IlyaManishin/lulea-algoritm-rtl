`timescale 1ns/1ps
`include "lulea_cache.svh"

module l3
    import lulea_config::*;
    import lulea_types::*;
    import lulea_cache_types::*;
(
    input  logic        clk,
    input  logic        rst,
    input  logic        valid_in,
    input  ipv4_parts_t ip,
    input  logic        is_ptr_in,
    input  l3_pc_idx_t  ptr,
    input  port_t       port_in,

    output logic        valid,
    output ipv4_parts_t ip_out,
    output logic        is_ptr,
    output port_t       payload,

    output l3_cache_req_t cache_req,
    input  l3_cache_rsp_t cache_rsp
);
    l3_word_addr_t word_addr;
    logic          lookup_en;

    logic          valid_pop;
    ipv4_parts_t   ip_pop;
    logic          is_ptr_pop, is_ptr_seq;
    port_t         port_pop, port_seq;
    pc_bit_pos_t   bit_pos;
    l3_pop_t       mask;
    popcount_t     offset;

    assign word_addr = {ptr, ip.l3.chunk_idx};
    assign bit_pos = ip_pop.l3.bit_pos;
    assign mask = '1 >> (PC_BIT_POS_MAX - int'(bit_pos));
    assign cache_req.pop_addr = word_addr;
    assign cache_req.chunk_addr = l3_chunk_addr_t'(word_addr);
    assign cache_req.seq_addr = l3_seq_addr_t'(cache_rsp.chunk_dout) + l3_seq_addr_t'(offset);

    assign lookup_en          = valid_in && is_ptr_in;
    assign cache_req.pop_en   = lookup_en;
    assign cache_req.chunk_en = lookup_en;
    assign cache_req.seq_en   = valid_pop && is_ptr_pop;

    assign is_ptr = 1'b0;
    assign payload = is_ptr_seq ? port_t'(cache_rsp.seq_dout) : port_seq;

    always_ff @(posedge clk) begin
        if (rst) begin
            valid_pop <= 1'b0;
            valid     <= 1'b0;
        end else begin
            valid_pop <= valid_in;
            valid     <= valid_pop;
        end

        ip_pop     <= ip;
        ip_out     <= ip_pop;
        is_ptr_pop <= is_ptr_in;
        is_ptr_seq <= is_ptr_pop;
        port_pop   <= port_in;
        port_seq   <= port_pop;
    end

    popcount #(
        .W(POP_WORD_WIDTH)
    ) u_popcount (
        .x(cache_rsp.pop_dout & mask),
        .cnt(offset)
    );
endmodule
