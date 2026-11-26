`timescale 1ns/1ps
`include "lulea_types.svh"

module l2
    import lulea_config::*;
    import lulea_types::*;
#(
    parameter string POP_INIT_FILE = "",
    parameter string CHUNK_INIT_FILE = "",
    parameter string SEQ_INIT_FILE = ""
) (
    input  logic          clk,
    input  logic          rst,
    input  logic          en,
    input  ipv4_parts_t   ip,
    input  logic          is_ptr_in,
    input  l2_word_addr_t ptr,
    input  port_t         port_in,

    output logic          valid,
    output ipv4_parts_t   ip_out,
    output logic          is_ptr,
    output l2_payload_t   payload
);
    logic          lookup_en;
    l2_word_addr_t word_addr;
    l2_pop_t       pop_word;
    l2_chunk_t     chunk_sum;

    logic          seq_en;
    l2_seq_addr_t  seq_addr;
    l2_seq_t       seq_cell;

    logic          valid_pop;
    ipv4_parts_t   ip_pop;
    logic          is_ptr_pop, is_ptr_seq;
    port_t         port_pop, port_seq;
    pc_bit_pos_t   bit_pos;
    l2_pop_t       mask;
    popcount_t     offset;

    assign lookup_en = en && is_ptr_in;
    assign word_addr = ptr;
    assign bit_pos = ip_pop.l2.bit_pos;
    assign mask = '1 >> (PC_BIT_POS_MAX - int'(bit_pos));
    assign seq_en = valid_pop && is_ptr_pop;
    assign seq_addr = l2_seq_addr_t'(chunk_sum) + l2_seq_addr_t'(offset);

    assign {is_ptr, payload} = is_ptr_seq ? seq_cell : {1'b0, port_seq};

    always_ff @(posedge clk) begin
        if (rst) begin
            valid_pop <= 1'b0;
            valid     <= 1'b0;
        end else begin
            valid_pop <= en;
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
        .x(pop_word & mask),
        .cnt(offset)
    );

    lulea_cache_rom #(
        .WORD_WIDTH(POP_WORD_WIDTH),
        .SIZE_BITS(L2_POP_SIZE_BITS),
        .INIT_FILE(POP_INIT_FILE)
    ) u_pop (
        .clk(clk),
        .en(lookup_en),
        .addr(word_addr),
        .dout(pop_word)
    );

    lulea_cache_rom #(
        .WORD_WIDTH(CHUNK_WORD_WIDTH),
        .SIZE_BITS(L2_CHUNK_SIZE_BITS),
        .INIT_FILE(CHUNK_INIT_FILE)
    ) u_chunk (
        .clk(clk),
        .en(lookup_en),
        .addr(word_addr),
        .dout(chunk_sum)
    );

    lulea_cache_rom #(
        .WORD_WIDTH(L2_SEQ_WORD_WIDTH),
        .SIZE_BITS(L2_SEQ_SIZE_BITS),
        .INIT_FILE(SEQ_INIT_FILE)
    ) u_seq (
        .clk(clk),
        .en(seq_en),
        .addr(seq_addr),
        .dout(seq_cell)
    );
endmodule
