`timescale 1ns/1ps
`include "lulea_cache.svh"

module top
    import lulea_types::*;
(
    input  logic  ref_clk,
    input  logic  rst,
    input  logic  en,
    input  ipv4_t ip,

    output logic  valid,
    output ipv4_t ip_out,
    output port_t payload
);
    lulea_cache_if cache();

    assign cache.en = !rst;

    lulea_cache u_cache (
        .clk(ref_clk),
        .cache(cache.memory)
    );

    pipeline u_pipeline (
        .ref_clk(ref_clk),
        .rst(rst),
        .en(en),
        .ip(ip),
        .valid(valid),
        .ip_out(ip_out),
        .payload(payload),
        .cache(cache.lookup)
    );
endmodule
