`timescale 1ns/1ps

module popcount_tester #(
    parameter int W = 32,
    parameter int NUM_RANDOM = 2000
) (
    input  logic clk,
    input  logic start,
    output logic done,
    output logic pass,
    output int   total_vectors,
    output int   error_count
);

    logic [W-1:0]           x;
    logic [$clog2(W+1)-1:0] cnt;

    popcount #(.W(W)) dut (
        .x   (x),
        .cnt (cnt)
    );

    function automatic int ref_popcount(input logic [W-1:0] val);
        int c;
        c = 0;
        for (int i = 0; i < W; i++) begin
            c += int'(val[i]);
        end
        return c;
    endfunction

    task automatic check_vector(input logic [W-1:0] val);
        int expected;
        x = val;
        #1;
        expected = ref_popcount(val);
        total_vectors++;

        // Also cross-check with $countones
        if (int'(cnt) !== expected || int'(cnt) !== $countones(val)) begin
            error_count++;
            pass = 1'b0;
            if (error_count <= 10) begin
                $display("[ERROR] W=%0d: x=0x%0h | dut_cnt=%0d | expected=%0d | $countones=%0d",
                         W, val, cnt, expected, $countones(val));
            end
        end
    endtask

    initial begin : blk_test
        int max_val;
        int density_pct;
        int r;

        done          = 1'b0;
        pass          = 1'b1;
        total_vectors = 0;
        error_count   = 0;
        x             = '0;

        @(posedge start);

        // 1. Directed Test: All Zeros
        check_vector('0);

        // 2. Directed Test: All Ones
        check_vector('1);

        // 3. Directed Test: Walking 1s (One-Hot)
        for (int i = 0; i < W; i++) begin
            logic [W-1:0] v;
            v = '0;
            v[i] = 1'b1;
            check_vector(v);
        end

        // 4. Directed Test: Walking 0s (One-Cold)
        for (int i = 0; i < W; i++) begin
            logic [W-1:0] v;
            v = '1;
            v[i] = 1'b0;
            check_vector(v);
        end

        // 5. Directed Test: Alternating patterns
        begin
            logic [W-1:0] alt01, alt10;
            for (int i = 0; i < W; i++) begin
                alt01[i] = (i % 2 == 1);
                alt10[i] = (i % 2 == 0);
            end
            check_vector(alt01);
            check_vector(alt10);
        end

        // 6. Directed Test: Alternating nibbles / bytes
        begin
            logic [W-1:0] pat_nib, pat_byte;
            for (int i = 0; i < W; i++) begin
                pat_nib[i]  = ((i / 4) % 2 == 1);
                pat_byte[i] = ((i / 8) % 2 == 1);
            end
            check_vector(pat_nib);
            check_vector(pat_byte);
        end

        // 7. Directed Test: Contiguous blocks of ones of varying length
        for (int len = 1; len < W; len++) begin
            logic [W-1:0] blk;
            blk = '0;
            for (int j = 0; j < len; j++) begin
                blk[j] = 1'b1;
            end
            check_vector(blk);
            check_vector(~blk);
        end

        // 8. Exhaustive test if W <= 12 (up to 4096 vectors)
        if (W <= 12) begin
            max_val = (1 << W);
            for (int v = 0; v < max_val; v++) begin
                check_vector(W'(v));
            end
        end else begin
            // 9. Randomized test with diverse bit densities
            for (int k = 0; k < NUM_RANDOM; k++) begin
                logic [W-1:0] rand_val;
                // Varying density: 10%, 25%, 50%, 75%, 90%
                density_pct = (k % 5 == 0) ? 10 :
                              (k % 5 == 1) ? 25 :
                              (k % 5 == 2) ? 50 :
                              (k % 5 == 3) ? 75 : 90;

                for (int bit_idx = 0; bit_idx < W; bit_idx++) begin
                    r = $urandom_range(1, 100);
                    rand_val[bit_idx] = (r <= density_pct);
                end
                check_vector(rand_val);
            end
        end

        done = 1'b1;
        if (pass) begin
            $display("[PASS] W=%3d | Vectors: %6d | Errors: 0", W, total_vectors);
        end else begin
            $display("[FAIL] W=%3d | Vectors: %6d | Errors: %0d", W, total_vectors, error_count);
        end
    end

endmodule
