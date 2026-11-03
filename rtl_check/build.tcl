# build.tcl - Recreate Vivado project and run implementation

set script_dir [file normalize [file dirname [info script]]]
set proj_name  "prj"
set proj_dir   [file join $script_dir "prj"]
set part       "xc7a35ticpg236-1L"
set top_module "top"

# Detect available CPU threads
if {[info exists ::env(JOBS)]} {
    set jobs $::env(JOBS)
} else {
    set jobs [expr {[info exists ::env(NUMBER_OF_PROCESSORS)] ? $::env(NUMBER_OF_PROCESSORS) : 8}]
}

# Create project
create_project -force $proj_name $proj_dir -part $part
set_property simulator_language Verilog [current_project]
set_property target_language Verilog [current_project]

# -----------------------------------------------------------------------------
# IP Core: blk_mem_gen_0 (Single Port ROM, 32768 x 18-bit)
# -----------------------------------------------------------------------------
create_ip -name blk_mem_gen -vendor xilinx.com -library ip -version 8.4 -module_name blk_mem_gen_0
set_property -dict [list \
    CONFIG.Memory_Type {Single_Port_ROM} \
    CONFIG.Write_Depth_A {32768} \
    CONFIG.Write_Width_A {18} \
] [get_ips blk_mem_gen_0]

# -----------------------------------------------------------------------------
# IP Core: blk_mem_gen_1 (Single Port ROM, 16384 x 36-bit)
# -----------------------------------------------------------------------------
create_ip -name blk_mem_gen -vendor xilinx.com -library ip -version 8.4 -module_name blk_mem_gen_1
set_property -dict [list \
    CONFIG.Memory_Type {Single_Port_ROM} \
    CONFIG.Write_Depth_A {16384} \
    CONFIG.Write_Width_A {36} \
] [get_ips blk_mem_gen_1]

# -----------------------------------------------------------------------------
# IP Core: blk_mem_gen_2 (Single Port ROM, 8192 x 72-bit)
# -----------------------------------------------------------------------------
create_ip -name blk_mem_gen -vendor xilinx.com -library ip -version 8.4 -module_name blk_mem_gen_2
set_property -dict [list \
    CONFIG.Memory_Type {Single_Port_ROM} \
    CONFIG.Write_Depth_A {8192} \
    CONFIG.Write_Width_A {72} \
] [get_ips blk_mem_gen_2]

# -----------------------------------------------------------------------------
# IP Core: clk_gen (MMCM: 100 MHz in -> 100, 150, 200 MHz out)
# -----------------------------------------------------------------------------
create_ip -name clk_wiz -vendor xilinx.com -library ip -version 6.0 -module_name clk_gen
set_property -dict [list \
    CONFIG.PRIMITIVE {MMCM} \
    CONFIG.NUM_OUT_CLKS {3} \
    CONFIG.CLK_OUT1_PORT {clk_100} \
    CONFIG.CLK_OUT2_PORT {clk_150} \
    CONFIG.CLK_OUT3_PORT {clk_200} \
    CONFIG.CLKOUT1_DRIVES {BUFG} \
    CONFIG.CLKOUT2_DRIVES {BUFG} \
    CONFIG.CLKOUT3_DRIVES {BUFG} \
    CONFIG.CLKOUT2_USED {true} \
    CONFIG.CLKOUT3_USED {true} \
    CONFIG.CLKOUT2_REQUESTED_OUT_FREQ {150} \
    CONFIG.CLKOUT3_REQUESTED_OUT_FREQ {200} \
    CONFIG.MMCM_BANDWIDTH {OPTIMIZED} \
    CONFIG.MMCM_CLKFBOUT_MULT_F {6.000} \
    CONFIG.MMCM_CLKIN1_PERIOD {10.0} \
    CONFIG.MMCM_CLKOUT0_DIVIDE_F {6.000} \
    CONFIG.MMCM_CLKOUT1_DIVIDE {4} \
    CONFIG.MMCM_CLKOUT2_DIVIDE {3} \
    CONFIG.MMCM_COMPENSATION {ZHOLD} \
    CONFIG.USE_INCLK_SWITCHOVER {false} \
] [get_ips clk_gen]

# -----------------------------------------------------------------------------
# Generate IP targets and Out-Of-Context synthesis runs
# -----------------------------------------------------------------------------
foreach ip [get_ips] {
    generate_target all [get_files [get_property IP_FILE $ip]]
    export_ip_user_files -of_objects [get_files [get_property IP_FILE $ip]] -no_script -sync -force -quiet
    create_ip_run [get_files -of_objects [get_fileset sources_1] [get_property IP_FILE $ip]]
}

launch_runs [get_runs *_synth_1] -jobs $jobs
foreach run [get_runs *_synth_1] {
    wait_on_run $run
}

# -----------------------------------------------------------------------------
# Add RTL sources and constraints
# -----------------------------------------------------------------------------
add_files -norecurse [file join $script_dir "src" "top.sv"]
add_files -fileset constrs_1 -norecurse [file join $script_dir "constraints" "rv_nsu_basys_3.xdc"]

update_compile_order -fileset sources_1
set_property top $top_module [current_fileset]

# -----------------------------------------------------------------------------
# RTL Lint Check & Implementation Flow
# -----------------------------------------------------------------------------
synth_design -top $top_module -part $part -lint

launch_runs synth_1 -jobs $jobs
wait_on_run synth_1

launch_runs impl_1 -jobs $jobs
wait_on_run impl_1

# Check implementation results
if {[get_property STATUS [get_runs impl_1]] ne "write_bitstream Complete!" && \
    [get_property STATUS [get_runs impl_1]] ne "route_design Complete!"} {
    puts "ERROR: Implementation failed."
    exit 1
}

# Generate reports
open_run impl_1
report_utilization -file [file join $proj_dir "utilization_impl.rpt"] -pb [file join $proj_dir "utilization_impl.pb"]
report_timing_summary -file [file join $proj_dir "timing_summary_impl.rpt"]

puts "INFO: Build finished successfully."