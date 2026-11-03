module top (
    input logic ref_clk,
    input logic rst
);
    logic clk_100, clk_150, clk_200;
    logic pll_locked;
    
    clk_gen clk_gen_inst(
        .reset(rst),
        .clk_in1(ref_clk),
        .clk_100(clk_100), 
        .clk_150(clk_150), 
        .clk_200(clk_200),
        .locked(pll_locked)
    );
    
    logic bram_clk;
    assign bram_clk = clk_100;
    
    blk_mem_gen_0 bram_18_inst (
    .clka(clka),
    .ena(ena),
    .addra(addra),
    .douta(douta)
  );
  
    blk_mem_gen_1 bram_36_inst (
    .clka(clka),
    .ena(ena),
    .addra(addra),
    .douta(douta)
  );
  
    blk_mem_gen_2 bram_72_inst (
    .clka(clka),
    .ena(ena),
    .addra(addra),
    .douta(douta)
  );



    
    (* dont_touch = "true" *) logic [6:0] test;
    
    always_ff @(posedge ref_clk) begin
        test <= '1;
    end


endmodule : top