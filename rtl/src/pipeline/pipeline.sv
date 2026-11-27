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
    assign cache.en = !rst;

    ipv4_parts_t ip_parts;
    assign ip_parts = ipv4_parts_t'(ip);

    // =========================================================================
    //  L1
    // =========================================================================

    logic        l1_valid;
    ipv4_parts_t l1_ip;
    logic        l1_is_ptr;
    l1_payload_t l1_payload;

    l1 u_l1 (
        .clk(ref_clk),
        .rst(rst),
        .valid_in(en),
        .ip(ip_parts),
        .valid(l1_valid),
        .ip_out(l1_ip),
        .is_ptr(l1_is_ptr),
        .payload(l1_payload),
        .cache_req(cache.l1_req),
        .cache_rsp(cache.l1_rsp)
    );

    // =========================================================================
    //  L2
    // =========================================================================

    logic        l2_valid;
    ipv4_parts_t l2_ip;
    logic        l2_is_ptr;
    l2_payload_t l2_payload;

    l2 u_l2 (
        .clk(ref_clk),
        .rst(rst),
        .valid_in(l1_valid),
        .ip(l1_ip),
        .is_ptr_in(l1_is_ptr),
        .ptr(l2_word_addr_t'(l1_payload)),
        .port_in(port_t'(l1_payload)),
        .valid(l2_valid),
        .ip_out(l2_ip),
        .is_ptr(l2_is_ptr),
        .payload(l2_payload),
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
        .is_ptr_in(l2_is_ptr),
        .ptr(l3_pc_idx_t'(l2_payload)),
        .port_in(port_t'(l2_payload)),
        .valid(l3_valid),
        .ip_out(l3_ip),
        .is_ptr(l3_is_ptr),
        .payload(l3_payload),
        .cache_req(cache.l3_req),
        .cache_rsp(cache.l3_rsp)
    );

    // =========================================================================
    //  OUTPUT
    // =========================================================================

    assign valid = l3_valid;
    assign ip_out = ipv4_t'(l3_ip);
    assign is_ptr = l3_is_ptr;
    assign payload = l3_payload;

endmodule
