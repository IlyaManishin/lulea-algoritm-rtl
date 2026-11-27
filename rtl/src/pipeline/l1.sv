`timescale 1ns/1ps
`include "lulea_cache.svh"

module l1
    import lulea_config::*;
    import lulea_types::*;
    import lulea_cache_types::*;
(
    input  logic          clk,
    input  logic          rst,
    input  logic          valid_in,
    input  ipv4_parts_t   ip,

    // -----Next level data-----------
    output logic          valid,
    output ipv4_parts_t   ip_out,
    output l2_word_addr_t ptr_out,

    // -----Pipeline early data-------
    output logic          port_valid,
    output port_t         port_out,

    // -----Lulea cache---------------
    output l1_cache_req_t cache_req,
    input  l1_cache_rsp_t cache_rsp
);

    logic          valid_pop1;
    logic          valid_pop2;
    logic          valid_seq;
    ipv4_parts_t   ip_pop1;
    ipv4_parts_t   ip_pop2;
    ipv4_parts_t   ip_seq;

    logic          seq_is_ptr;

    // =========================================================================
    //  Lulea Lookup
    // =========================================================================

    l1_word_addr_t word_addr;
    l1_pop_t       mask;
    popcount_t     offset;
    pc_bit_pos_t   bit_pos;

    // ---- Stage pop1 ----
    assign word_addr = ip.l1.chunk_idx;

    assign cache_req.pop_addr = word_addr;
    assign cache_req.chunk_addr = word_addr;

    // ---- Stage pop2 ----
    l1_pop_t masked_word;
    popcount_half_t offset_left, offset_left_r;
    popcount_half_t offset_right, offset_right_r;
    l1_chunk_t chunk_sum_r;

    assign bit_pos = ip_pop1.l1.bit_pos;
    assign mask = '1 >> (PC_BIT_POS_MAX - int'(bit_pos));
    assign masked_word = cache_rsp.pop_dout & mask;

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

    // ---- Stage seq ----
    assign offset = popcount_t'(offset_left_r) + popcount_t'(offset_right_r);
    assign cache_req.seq_addr = l1_seq_addr_t'(chunk_sum_r) + l1_seq_addr_t'(offset);

    // =========================================================================
    //  Output
    // =========================================================================

    assign seq_is_ptr = cache_rsp.seq_dout[L1_PAYLOAD_WIDTH];
    assign ptr_out = l2_word_addr_t'(cache_rsp.seq_dout);
    assign port_valid = valid_seq && !seq_is_ptr;
    assign port_out = port_t'(cache_rsp.seq_dout);

    assign valid = valid_seq && seq_is_ptr;
    assign ip_out = ip_seq;


    // =========================================================================
    //  Pipeline
    // =========================================================================

    always_ff @(posedge clk) begin
        if (rst) begin
            valid_pop1 <= 1'b0;
            valid_pop2 <= 1'b0;
            valid_seq <= 1'b0;
        end else begin
            valid_pop1 <= valid_in;
            valid_pop2 <= valid_pop1;
            valid_seq <= valid_pop2;
        end

        ip_pop1 <= ip;
        ip_pop2 <= ip_pop1;
        ip_seq <= ip_pop2;

        offset_left_r  <= offset_left;
        offset_right_r <= offset_right;
        chunk_sum_r    <= cache_rsp.chunk_dout;
    end

    assign cache_req.pop_en   = valid_in;
    assign cache_req.chunk_en = valid_in;
    assign cache_req.seq_en   = valid_pop2;
endmodule
