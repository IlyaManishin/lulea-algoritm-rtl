`timescale 1ns/1ps
`include "lulea_types.svh"

module pipeline
    import lulea_types::*;
(
    input  logic  ref_clk,
    input  logic  rst,
    input  logic  en,
    input  ipv4_t ip,

    output logic  valid,
    output ipv4_t ip_out,
    output logic  is_ptr,
    output port_t payload
);
    ipv4_parts_t  ip_parts;
    ipv4_parts_t  l3_ip;

    logic         l1_valid;
    ipv4_parts_t  l1_ip;
    logic         l1_is_ptr;
    l1_payload_t  l1_payload;

    logic         l2_valid;
    ipv4_parts_t  l2_ip;
    logic         l2_is_ptr;
    l2_payload_t  l2_payload;

    assign ip_parts = ipv4_parts_t'(ip);
    assign ip_out = ipv4_t'(l3_ip);

    (* DONT_TOUCH = "TRUE" *)
    l1 #(
        .POP_INIT_FILE("l1_pop.mem"),
        .CHUNK_INIT_FILE("l1_chunk.mem"),
        .SEQ_INIT_FILE("l1_seq.mem")
    ) u_l1 (
        .clk(ref_clk),
        .rst(rst),
        .en(en),
        .ip(ip_parts),
        .valid(l1_valid),
        .ip_out(l1_ip),
        .is_ptr(l1_is_ptr),
        .payload(l1_payload)
    );

    (* DONT_TOUCH = "TRUE" *)
    l2 #(
        .POP_INIT_FILE("l2_pop.mem"),
        .CHUNK_INIT_FILE("l2_chunk.mem"),
        .SEQ_INIT_FILE("l2_seq.mem")
    ) u_l2 (
        .clk(ref_clk),
        .rst(rst),
        .en(l1_valid),
        .ip(l1_ip),
        .is_ptr_in(l1_is_ptr),
        .ptr(l2_word_addr_t'(l1_payload)),
        .port_in(port_t'(l1_payload)),
        .valid(l2_valid),
        .ip_out(l2_ip),
        .is_ptr(l2_is_ptr),
        .payload(l2_payload)
    );

    (* DONT_TOUCH = "TRUE" *)
    l3 #(
        .POP_INIT_FILE("l3_pop.mem"),
        .CHUNK_INIT_FILE("l3_chunk.mem"),
        .SEQ_INIT_FILE("l3_seq.mem")
    ) u_l3 (
        .clk(ref_clk),
        .rst(rst),
        .en(l2_valid),
        .ip(l2_ip),
        .is_ptr_in(l2_is_ptr),
        .ptr(l3_pc_idx_t'(l2_payload)),
        .port_in(port_t'(l2_payload)),
        .valid(valid),
        .ip_out(l3_ip),
        .is_ptr(is_ptr),
        .payload(payload)
    );
endmodule
