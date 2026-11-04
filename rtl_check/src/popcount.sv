`timescale 1ns/1ps

module popcount #(
    parameter int W = 32
) (
    input  logic [W-1:0]           x,
    output logic [$clog2(W+1)-1:0] cnt
);
    if (W <= 6) begin : g_leaf
        always_comb begin
            cnt = '0;
            for (int i = 0; i < W; i++)
                cnt = cnt + x[i];
        end
    end else begin : g_node
        localparam int NL = (W + 5) / 6;
        localparam int WL = 6 * ((NL + 1) / 2);
        localparam int WR = W - WL;

        logic [$clog2(WL+1)-1:0] cnt_l;
        logic [$clog2(WR+1)-1:0] cnt_r;

        popcount #(.W(WL)) u_l (.x(x[WL-1:0]), .cnt(cnt_l));
        popcount #(.W(WR)) u_r (.x(x[W-1:WL]), .cnt(cnt_r));

        assign cnt = cnt_l + cnt_r;
    end
endmodule