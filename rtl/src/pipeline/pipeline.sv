`timescale 1ns / 1ps
`include "lulea_cache.svh"

module pipeline
  import lulea_types::*;
(
    input logic  ref_clk,
    input logic  rst,
    input logic  en,
    input ipv4_t ip,

    output logic  valid,
    output ipv4_t ip_out,
    output logic  is_ptr,
    output port_t payload,

    lulea_cache_if.lookup cache
);
    ipv4_parts_t ip_parts;
    assign ip_parts = ipv4_parts_t'(ip);

    // =========================================================================
    //  L1
    // =========================================================================

    logic          l1_valid;
    ipv4_parts_t   l1_ip;
    l2_word_addr_t l1_ptr;
    logic          l1_port_valid;
    port_t         l1_port;

    l1 u_l1 (
        .clk(ref_clk),
        .rst(rst),

        .valid_in(en),
        .ip(ip_parts),
        .valid(l1_valid),
        .ip_out(l1_ip),
        .ptr_out(l1_ptr),
        .port_valid(l1_port_valid),
        .port_out(l1_port),

        .cache_req(cache.l1_req),
        .cache_rsp(cache.l1_rsp)
    );

    // =========================================================================
    //  L2
    // =========================================================================

    logic        l2_valid;
    ipv4_parts_t l2_ip;
    l3_pc_idx_t  l2_ptr;
    logic        l2_port_valid;
    port_t       l2_port;

    l2 u_l2 (
        .clk(ref_clk),
        .rst(rst),

        .valid_in(l1_valid),
        .ip(l1_ip),
        .ptr(l1_ptr),
        .valid(l2_valid),
        .ip_out(l2_ip),
        .ptr_out(l2_ptr),
        .port_valid(l2_port_valid),
        .port_out(l2_port),

        .cache_req(cache.l2_req),
        .cache_rsp(cache.l2_rsp)
    );

    // =========================================================================
    //  L3
    // =========================================================================

    ipv4_parts_t l3_ip;
    logic        l3_valid;
    logic        l3_is_ptr;
    port_t       l3_payload;

    l3 u_l3 (
        .clk(ref_clk),
        .rst(rst),

        .valid_in(l2_valid),
        .ip(l2_ip),
        .ptr(l2_ptr),
        .valid(l3_valid),
        .ip_out(l3_ip),
        .is_ptr(l3_is_ptr),
        .payload(l3_payload),

        .cache_req(cache.l3_req),
        .cache_rsp(cache.l3_rsp)
    );

    // =========================================================================
    //  RESULT PORT PIPELINE
    // =========================================================================

    typedef struct packed {
        logic        valid;
        ipv4_parts_t ip;
        port_t       payload;
    } result_t;

    result_t l1_result_st1, l1_result_st2;
    result_t early_result_st1, early_result_st2;

    // Keep result ip-port pair until it reach end of pipeline
    always_ff @(posedge ref_clk) begin
        if (rst) begin
            l1_result_st1.valid    <= 1'b0;
            l1_result_st2.valid    <= 1'b0;
            early_result_st1.valid <= 1'b0;
            early_result_st2.valid <= 1'b0;
        end else begin
            l1_result_st1 <= '{
                valid: l1_port_valid,
                ip: l1_ip,
                payload: l1_port
            };
            l1_result_st2 <= l1_result_st1;

            early_result_st1 <= l1_result_st2.valid ? l1_result_st2 : '{
                valid: l2_port_valid,
                ip: l2_ip,
                payload: l2_port
            };
            early_result_st2 <= early_result_st1;
        end
    end

    // =========================================================================
    //  OUTPUT
    // =========================================================================

    assign valid = early_result_st2.valid || l3_valid;
    assign ip_out = ipv4_t'(early_result_st2.valid ? early_result_st2.ip : l3_ip);
    assign is_ptr = l3_is_ptr;
    assign payload = early_result_st2.valid ? early_result_st2.payload : l3_payload;

endmodule
