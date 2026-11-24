#!/bin/bash
set -e

RTL_DIR=$(realpath "$(dirname "$0")")
LOG_DIR="$RTL_DIR/log"
mkdir -p "$LOG_DIR"

exec vivado -mode batch \
    -journal "$LOG_DIR/bld.jou" \
    -log "$LOG_DIR/bld.log" \
    -source "$RTL_DIR/build.tcl" \
    -notrace
