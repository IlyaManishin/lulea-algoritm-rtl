`timescale 1ns/1ps

module lulea_cache_rom #(
    parameter int unsigned WORD_WIDTH = 64,
    parameter int unsigned SIZE_BITS = 1024,
    parameter string INIT_FILE = "",

    localparam int unsigned DEPTH = SIZE_BITS / WORD_WIDTH,
    localparam int unsigned ADDR_WIDTH = (DEPTH > 1) ? $clog2(DEPTH) : 1
) (
    input  logic                  clk,
    input  logic                  en,
    input  logic [ADDR_WIDTH-1:0] addr,
    output logic [WORD_WIDTH-1:0] dout
);
    if (WORD_WIDTH == 0 || DEPTH == 0 || SIZE_BITS % WORD_WIDTH != 0) begin : g_invalid_size
        initial $fatal(1, "lulea_cache_rom: SIZE_BITS must hold a positive whole number of words");
    end

    (* ram_style = "block", ram_decomp = "area" *) logic [WORD_WIDTH-1:0] rom [0:DEPTH-1];

    initial begin
        if (INIT_FILE != "") begin
            $readmemb(INIT_FILE, rom, 0, DEPTH-1);
        end else begin
            $warning("%m: INIT_FILE is empty. ROM contents are initialized to zero.");
            for (int i = 0; i < DEPTH; i++)
                rom[i] = '0;
        end
    end

    always_ff @(posedge clk) begin
        if (en)
            dout <= rom[addr];
    end
endmodule
