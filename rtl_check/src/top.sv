// Configurations:
// `define USE_BRAM_16
// `define USE_BRAM_32
// Default is USE_BRAM_64

// Enable pipeline register between BRAM output and popcount logic:
// `define IS_PIPELINED

module top (
    input  logic ref_clk,
    input  logic rst
);

    logic clk_100, clk_150, clk_200;
    logic pll_locked;

    clk_gen clk_gen_inst (
        .reset    (rst),
        .clk_in1  (ref_clk),
        .clk_100  (clk_100),
        .clk_150  (clk_150),
        .clk_200  (clk_200),
        .locked   (pll_locked)
    );

    logic bram_clk;
`ifdef USE_CLK_200
    assign bram_clk = clk_200;
`else
    assign bram_clk = clk_100;
`endif

`ifdef USE_BRAM_16
    logic [15:0] douta;

    blk_mem_gen_0 bram_16_inst (
        .clka  (bram_clk),
        .ena   (1'b1),
        .addra ('0),
        .douta (douta)
    );
`elsif USE_BRAM_32
    logic [31:0] douta;

    blk_mem_gen_1 bram_32_inst (
        .clka  (bram_clk),
        .ena   (1'b1),
        .addra ('0),
        .douta (douta)
    );
`else // Default: USE_BRAM_64
    logic [63:0] douta;

    blk_mem_gen_2 bram_64_inst (
        .clka  (bram_clk),
        .ena   (1'b1),
        .addra ('0),
        .douta (douta)
    );
`endif

    (* dont_touch = "true" *) logic [6:0] test;

`ifdef IS_PIPELINED
    // Pipeline register to isolate BRAM read routing from popcount tree
    (* dont_touch = "true" *) logic [$bits(douta)-1:0] douta_pipe;

    always_ff @(posedge bram_clk or posedge rst) begin
        if (rst) begin
            douta_pipe <= '0;
            test       <= '0;
        end else if (pll_locked) begin
            douta_pipe <= douta;
            test       <= $countones(douta_pipe);
        end
    end
`else
    // Combinational popcount directly on BRAM read output
    always_ff @(posedge bram_clk or posedge rst) begin
        if (rst) begin
            test <= '0;
        end else if (pll_locked) begin
            test <= $countones(douta);
        end
    end
`endif

endmodule : top