`ifndef LULEA_CONFIG_SVH
`define LULEA_CONFIG_SVH

package lulea_config;
    // Bank capacities use the selected allocation from stats/cache_stats/TOTALS.md.

    // =========================================================================
    //  LULEA 18-6-8 BASE CONFIGURATION
    // =========================================================================

    // ------------------- IP AND PORT WIDTH -------------------
    localparam int unsigned L1_IP_WIDTH = 18;
    localparam int unsigned L2_IP_WIDTH = 6;
    localparam int unsigned L3_IP_WIDTH = 8;
    localparam int unsigned IP_WIDTH = L1_IP_WIDTH + L2_IP_WIDTH + L3_IP_WIDTH;
    localparam int unsigned PORT_WIDTH = 8;

    // ------------------- RAM WIDTH -------------------
    localparam int unsigned POP_WORD_WIDTH = 64;
    localparam int unsigned POP_WORD_HALF_WIDTH = 32;
    localparam int unsigned CHUNK_WORD_WIDTH = 18;

    // ------------------- BIT POSITION AND POPCOUNT -------------------
    localparam int unsigned PC_BIT_POS_WIDTH = $clog2(POP_WORD_WIDTH);
    localparam int unsigned PC_BIT_POS_MAX = POP_WORD_WIDTH - 1;
    localparam int unsigned POPCOUNT_WIDTH = $clog2(POP_WORD_WIDTH + 1);

    // ------------------- IP FIELD WIDTHS -------------------
    localparam int unsigned L1_IP_CHUNK_IDX_WIDTH = L1_IP_WIDTH - PC_BIT_POS_WIDTH;
    localparam int unsigned L2_IP_BIT_POS_WIDTH = L2_IP_WIDTH;
    localparam int unsigned L3_IP_CHUNK_IDX_WIDTH = L3_IP_WIDTH - PC_BIT_POS_WIDTH;

    // =========================================================================
    //  L1 level  (18 bits: /1 -> /18)
    // =========================================================================

    // ------------------- BRAM SIZE -------------------
    localparam int unsigned L1_POP_SIZE_BITS = 262_144;   // 16 RAMB18
    localparam int unsigned L1_CHUNK_SIZE_BITS = 73_728;  // 4 RAMB18
    localparam int unsigned L1_SEQ_SIZE_BITS = 294_912;   // 16 RAMB18

    // ------------------- RAM WIDTH -------------------
    localparam int unsigned L1_SEQ_WORD_WIDTH = 18;
    localparam int unsigned L1_PAYLOAD_WIDTH = L1_SEQ_WORD_WIDTH - 1;

    // ------------------- RAM DEPTH -------------------
    localparam int unsigned L1_POP_DEPTH = L1_POP_SIZE_BITS / POP_WORD_WIDTH;
    localparam int unsigned L1_CHUNK_DEPTH = L1_CHUNK_SIZE_BITS / CHUNK_WORD_WIDTH;
    localparam int unsigned L1_SEQ_DEPTH = L1_SEQ_SIZE_BITS / L1_SEQ_WORD_WIDTH;

    // ------------------- RAM ADDR -------------------
    localparam int unsigned L1_WORD_ADDR_WIDTH = $clog2(L1_POP_DEPTH);
    localparam int unsigned L1_SEQ_ADDR_WIDTH = $clog2(L1_SEQ_DEPTH);

    // =========================================================================
    //  L2 level  (6 bits: /19 -> /24)
    // =========================================================================

    // ------------------- BRAM SIZE -------------------
    localparam int unsigned L2_POP_SIZE_BITS = 524_288;    // 32 RAMB18
    localparam int unsigned L2_CHUNK_SIZE_BITS = 147_456;  // 8 RAMB18
    localparam int unsigned L2_SEQ_SIZE_BITS = 184_320;    // 10 RAMB18

    // ------------------- RAM WIDTH -------------------
    localparam int unsigned L2_SEQ_WORD_WIDTH = 9;
    localparam int unsigned L2_PAYLOAD_WIDTH = L2_SEQ_WORD_WIDTH - 1;

    // ------------------- RAM DEPTH -------------------
    localparam int unsigned L2_POP_DEPTH = L2_POP_SIZE_BITS / POP_WORD_WIDTH;
    localparam int unsigned L2_CHUNK_DEPTH = L2_CHUNK_SIZE_BITS / CHUNK_WORD_WIDTH;
    localparam int unsigned L2_SEQ_DEPTH = L2_SEQ_SIZE_BITS / L2_SEQ_WORD_WIDTH;

    // ------------------- RAM ADDR -------------------
    localparam int unsigned L2_PC_IDX_WIDTH = $clog2(L2_POP_DEPTH);
    localparam int unsigned L2_WORD_ADDR_WIDTH = L2_PC_IDX_WIDTH;
    localparam int unsigned L2_SEQ_ADDR_WIDTH = $clog2(L2_SEQ_DEPTH);

    // =========================================================================
    //  L3 level  (8 bits: /25 -> /32)
    // =========================================================================

    // ------------------- BRAM SIZE -------------------
    localparam int unsigned L3_POP_SIZE_BITS = 32_768;    // 2 RAMB18
    localparam int unsigned L3_CHUNK_SIZE_BITS = 18_432;  // 1 RAMB18
    localparam int unsigned L3_SEQ_SIZE_BITS = 18_432;    // 1 RAMB18

    // ------------------- RAM WIDTH -------------------
    localparam int unsigned L3_SEQ_WORD_WIDTH = 9;

    // ------------------- RAM DEPTH -------------------
    localparam int unsigned L3_POP_DEPTH = L3_POP_SIZE_BITS / POP_WORD_WIDTH;
    localparam int unsigned L3_CHUNK_DEPTH = L3_CHUNK_SIZE_BITS / CHUNK_WORD_WIDTH;
    localparam int unsigned L3_SEQ_DEPTH = L3_SEQ_SIZE_BITS / L3_SEQ_WORD_WIDTH;

    // ------------------- RAM ADDR -------------------
    localparam int unsigned L3_WORD_ADDR_WIDTH = $clog2(L3_POP_DEPTH);
    localparam int unsigned L3_PC_IDX_WIDTH = L3_WORD_ADDR_WIDTH - L3_IP_CHUNK_IDX_WIDTH;
    localparam int unsigned L3_CHUNK_ADDR_WIDTH = $clog2(L3_CHUNK_DEPTH);
    localparam int unsigned L3_SEQ_ADDR_WIDTH = $clog2(L3_SEQ_DEPTH);
endpackage

`endif
