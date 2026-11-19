`timescale 1ns/1ps

module tb_popcount;

    logic clk;
    logic start;

    initial begin
        clk = 1'b0;
        forever #5 clk = ~clk;
    end

    // Tested widths:
    // Leaves: 1, 2, 3, 4, 5, 6
    // Tree boundaries: 7, 8, 11, 12, 13, 14
    // Project widths: 16, 32, 64
    // Non-standard / large: 18, 25, 36, 48, 72, 100, 128
    localparam int NUM_TESTS = 20;

    logic [NUM_TESTS-1:0] done_bus;
    logic [NUM_TESTS-1:0] pass_bus;
    int total_vecs[NUM_TESTS];
    int total_errs[NUM_TESTS];

    // Instantiate testers
    popcount_tester #(.W(1))   u_t1   (.clk(clk), .start(start), .done(done_bus[0]),  .pass(pass_bus[0]),  .total_vectors(total_vecs[0]),  .error_count(total_errs[0]));
    popcount_tester #(.W(2))   u_t2   (.clk(clk), .start(start), .done(done_bus[1]),  .pass(pass_bus[1]),  .total_vectors(total_vecs[1]),  .error_count(total_errs[1]));
    popcount_tester #(.W(3))   u_t3   (.clk(clk), .start(start), .done(done_bus[2]),  .pass(pass_bus[2]),  .total_vectors(total_vecs[2]),  .error_count(total_errs[2]));
    popcount_tester #(.W(4))   u_t4   (.clk(clk), .start(start), .done(done_bus[3]),  .pass(pass_bus[3]),  .total_vectors(total_vecs[3]),  .error_count(total_errs[3]));
    popcount_tester #(.W(5))   u_t5   (.clk(clk), .start(start), .done(done_bus[4]),  .pass(pass_bus[4]),  .total_vectors(total_vecs[4]),  .error_count(total_errs[4]));
    popcount_tester #(.W(6))   u_t6   (.clk(clk), .start(start), .done(done_bus[5]),  .pass(pass_bus[5]),  .total_vectors(total_vecs[5]),  .error_count(total_errs[5]));
    popcount_tester #(.W(7))   u_t7   (.clk(clk), .start(start), .done(done_bus[6]),  .pass(pass_bus[6]),  .total_vectors(total_vecs[6]),  .error_count(total_errs[6]));
    popcount_tester #(.W(8))   u_t8   (.clk(clk), .start(start), .done(done_bus[7]),  .pass(pass_bus[7]),  .total_vectors(total_vecs[7]),  .error_count(total_errs[7]));
    popcount_tester #(.W(11))  u_t11  (.clk(clk), .start(start), .done(done_bus[8]),  .pass(pass_bus[8]),  .total_vectors(total_vecs[8]),  .error_count(total_errs[8]));
    popcount_tester #(.W(12))  u_t12  (.clk(clk), .start(start), .done(done_bus[9]),  .pass(pass_bus[9]),  .total_vectors(total_vecs[9]),  .error_count(total_errs[9]));
    popcount_tester #(.W(13))  u_t13  (.clk(clk), .start(start), .done(done_bus[10]), .pass(pass_bus[10]), .total_vectors(total_vecs[10]), .error_count(total_errs[10]));
    popcount_tester #(.W(14))  u_t14  (.clk(clk), .start(start), .done(done_bus[11]), .pass(pass_bus[11]), .total_vectors(total_vecs[11]), .error_count(total_errs[11]));
    popcount_tester #(.W(16))  u_t16  (.clk(clk), .start(start), .done(done_bus[12]), .pass(pass_bus[12]), .total_vectors(total_vecs[12]), .error_count(total_errs[12]));
    popcount_tester #(.W(18))  u_t18  (.clk(clk), .start(start), .done(done_bus[13]), .pass(pass_bus[13]), .total_vectors(total_vecs[13]), .error_count(total_errs[13]));
    popcount_tester #(.W(25))  u_t25  (.clk(clk), .start(start), .done(done_bus[14]), .pass(pass_bus[14]), .total_vectors(total_vecs[14]), .error_count(total_errs[14]));
    popcount_tester #(.W(32))  u_t32  (.clk(clk), .start(start), .done(done_bus[15]), .pass(pass_bus[15]), .total_vectors(total_vecs[15]), .error_count(total_errs[15]));
    popcount_tester #(.W(36))  u_t36  (.clk(clk), .start(start), .done(done_bus[16]), .pass(pass_bus[16]), .total_vectors(total_vecs[16]), .error_count(total_errs[16]));
    popcount_tester #(.W(48))  u_t48  (.clk(clk), .start(start), .done(done_bus[17]), .pass(pass_bus[17]), .total_vectors(total_vecs[17]), .error_count(total_errs[17]));
    popcount_tester #(.W(64))  u_t64  (.clk(clk), .start(start), .done(done_bus[18]), .pass(pass_bus[18]), .total_vectors(total_vecs[18]), .error_count(total_errs[18]));
    popcount_tester #(.W(72))  u_t72  (.clk(clk), .start(start), .done(done_bus[19]), .pass(pass_bus[19]), .total_vectors(total_vecs[19]), .error_count(total_errs[19]));

    initial begin : blk_main
        int sum_vectors;
        int sum_errors;

        sum_vectors = 0;
        sum_errors  = 0;

        $display("================================================================================");
        $display("  STARTING SYSTEMVERILOG POPCOUNT VERIFICATION SUITE");
        $display("  Testing 20 widths: W = 1..8, 11..14, 16, 18, 25, 32, 36, 48, 64, 72");
        $display("================================================================================");

        start = 1'b0;
        #10;
        start = 1'b1;

        // Wait for all testers to complete
        wait (&done_bus);
        #10;

        for (int i = 0; i < NUM_TESTS; i++) begin
            sum_vectors += total_vecs[i];
            sum_errors  += total_errs[i];
        end

        $display("================================================================================");
        $display("  VERIFICATION SUMMARY");
        $display("  Total test configurations: %0d", NUM_TESTS);
        $display("  Total stimulus vectors:   %0d", sum_vectors);
        $display("  Total mismatch errors:    %0d", sum_errors);
        if (&pass_bus && sum_errors == 0) begin
            $display("  STATUS: ALL TESTS PASSED SUCCESSFULLY! [SUCCESS]");
        end else begin
            $display("  STATUS: SOME TESTS FAILED! [FAILURE]");
        end
        $display("================================================================================");

        if (sum_errors != 0) begin
            $fatal(1, "Popcount verification encountered %0d errors!", sum_errors);
        end else begin
            $finish;
        end
    end

endmodule
