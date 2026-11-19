#!/usr/bin/env bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC_DIR="$SCRIPT_DIR/../src"
SIM_DIR="$SCRIPT_DIR/xsim_dir"

mkdir -p "$SIM_DIR"
cd "$SIM_DIR"

VIVADO_BIN="/home/mashina/soft/xilinx/2026.1/Vivado/bin"
export PATH="$VIVADO_BIN:$PATH"

echo "================================================================================"
echo "Compiling and running SystemVerilog popcount testbench with Vivado xsim..."
echo "================================================================================"

xvlog -sv "$SRC_DIR/popcount.sv" "$SCRIPT_DIR/popcount_tester.sv" "$SCRIPT_DIR/tb_popcount.sv"
xelab -top tb_popcount -s sim -debug off
xsim sim -R
