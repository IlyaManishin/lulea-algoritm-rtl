`timescale 1ns/1ps
`include "lulea_cache.svh"

module l2
    import lulea_config::*;
    import lulea_types::*;
    import lulea_cache_types::*;
(
    input  logic          clk,
    input  logic          rst,
    input  logic          valid_in,
    input  ipv4_parts_t   ip,
    input  l2_word_addr_t ptr,

    // -----Next level data-----------
    output logic          valid,
    output ipv4_parts_t   ip_out,
    output l3_pc_idx_t    ptr_out,

    // -----Pipeline early data-------
    output logic          port_valid,
    output port_t         port_out,

    // -----Lulea cache---------------
    output l2_cache_req_t cache_req,
    input  l2_cache_rsp_t cache_rsp
);

    logic          valid_pop;
    logic          valid_addr;
    logic          valid_seq;
    ipv4_parts_t   ip_pop;
    ipv4_parts_t   ip_addr;
    ipv4_parts_t   ip_seq;

    // =========================================================================
    //  Lulea Lookup
    // =========================================================================

    l2_word_addr_t word_addr;
    l2_seq_addr_t  seq_addr;

    // ---- Stage pop ----
    assign word_addr = ptr;

    assign cache_req.pop_addr = word_addr;
    assign cache_req.chunk_addr = word_addr;

    lulea_seq_addr #(
        .SEQ_ADDR_WIDTH(L2_SEQ_ADDR_WIDTH)
    ) u_seq_addr (
        .clk(clk),
        .rst(rst),
        .valid_in(valid_pop),
        .ip(ip_pop),
        .bit_pos(ip_pop.l2.bit_pos),
        .pop_word(cache_rsp.pop_dout),
        .chunk_sum(cache_rsp.chunk_dout),
        .valid(valid_addr),
        .ip_out(ip_addr),
        .seq_addr(seq_addr)
    );
    // ---- Stage seq ----
    assign cache_req.seq_addr = seq_addr;

    // =========================================================================
    //  Output
    // =========================================================================

    logic seq_is_ptr;

    assign seq_is_ptr = cache_rsp.seq_dout[L2_PAYLOAD_WIDTH];
    assign ptr_out = l3_pc_idx_t'(cache_rsp.seq_dout);
    assign port_valid = valid_seq && !seq_is_ptr;
    assign port_out = port_t'(cache_rsp.seq_dout);

    assign valid = valid_seq && seq_is_ptr;
    assign ip_out = ip_seq;

    // =========================================================================
    //  Pipeline
    // =========================================================================

    always_ff @(posedge clk) begin
        if (rst) begin
            valid_pop <= 1'b0;
            valid_seq <= 1'b0;
        end else begin
            valid_pop <= valid_in;
            valid_seq <= valid_addr;
        end

        ip_pop <= ip;
        ip_seq <= ip_addr;
    end

    assign cache_req.pop_en   = valid_in;
    assign cache_req.chunk_en = valid_in;
    assign cache_req.seq_en   = valid_addr;
endmodule
