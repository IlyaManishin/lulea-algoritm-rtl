# run_experiments.tcl - Run 12 design variations and extract timing/resource reports

set script_dir [file normalize [file dirname [info script]]]
set proj_dir   [file normalize [file join $script_dir ".." "prj1"]]
set results_dir [file join $script_dir "results"]

file mkdir $results_dir

if {[info exists ::env(JOBS)]} {
    set jobs $::env(JOBS)
} else {
    set jobs [expr {[info exists ::env(NUMBER_OF_PROCESSORS)] ? $::env(NUMBER_OF_PROCESSORS) : 8}]
}

# Open existing project
open_project [file join $proj_dir "prj1.xpr"]

set configs {
    {"run_01_bram16_nopipe_100mhz"  "USE_BRAM_16"  ""              "USE_CLK_100" 0}
    {"run_02_bram16_nopipe_200mhz"  "USE_BRAM_16"  ""              "USE_CLK_200" 0}
    {"run_03_bram16_pipe_100mhz"    "USE_BRAM_16"  "IS_PIPELINED"  "USE_CLK_100" 1}
    {"run_04_bram16_pipe_200mhz"    "USE_BRAM_16"  "IS_PIPELINED"  "USE_CLK_200" 1}
    {"run_05_bram32_nopipe_100mhz"  "USE_BRAM_32"  ""              "USE_CLK_100" 0}
    {"run_06_bram32_nopipe_200mhz"  "USE_BRAM_32"  ""              "USE_CLK_200" 0}
    {"run_07_bram32_pipe_100mhz"    "USE_BRAM_32"  "IS_PIPELINED"  "USE_CLK_100" 1}
    {"run_08_bram32_pipe_200mhz"    "USE_BRAM_32"  "IS_PIPELINED"  "USE_CLK_200" 1}
    {"run_09_bram64_nopipe_100mhz"  "USE_BRAM_64"  ""              "USE_CLK_100" 0}
    {"run_10_bram64_nopipe_200mhz"  "USE_BRAM_64"  ""              "USE_CLK_200" 0}
    {"run_11_bram64_pipe_100mhz"    "USE_BRAM_64"  "IS_PIPELINED"  "USE_CLK_100" 1}
    {"run_12_bram64_pipe_200mhz"    "USE_BRAM_64"  "IS_PIPELINED"  "USE_CLK_200" 1}
}

foreach cfg $configs {
    set run_name    [lindex $cfg 0]
    set bram_def    [lindex $cfg 1]
    set pipe_def    [lindex $cfg 2]
    set clk_def     [lindex $cfg 3]
    set is_pipe     [lindex $cfg 4]

    set out_dir [file join $results_dir $run_name]
    file mkdir $out_dir

    puts "================================================================================"
    puts ">>> Starting configuration: $run_name"
    puts "    Defines: $bram_def $pipe_def $clk_def"
    puts "================================================================================"

    set def_list [list]
    if {$bram_def ne ""} { lappend def_list $bram_def }
    if {$pipe_def ne ""} { lappend def_list $pipe_def }
    if {$clk_def  ne ""} { lappend def_list $clk_def }

    set_property verilog_define $def_list [get_filesets sources_1]

    # Run Synthesis
    reset_run synth_1
    launch_runs synth_1 -jobs $jobs
    wait_on_run synth_1

    if {[get_property STATUS [get_runs synth_1]] ne "synth_design Complete!"} {
        puts "ERROR: synth_1 failed for $run_name"
        continue
    }

    # Run Implementation
    reset_run impl_1
    launch_runs impl_1 -jobs $jobs
    wait_on_run impl_1

    if {[get_property STATUS [get_runs impl_1]] ne "write_bitstream Complete!" && \
        [get_property STATUS [get_runs impl_1]] ne "route_design Complete!"} {
        puts "WARNING: impl_1 ended with status [get_property STATUS [get_runs impl_1]] for $run_name"
    }

    # Open Implemented Design and report
    open_run impl_1

    # 1. Full timing summary
    report_timing_summary -file [file join $out_dir "timing_summary.rpt"] -max_paths 10

    # 2. Critical path to test_reg
    report_timing -to [get_cells -hierarchical -filter {NAME =~ test_reg*}] \
        -file [file join $out_dir "timing_to_test.rpt"] \
        -max_paths 10 -nworst 1 -path_type full_clock_expanded

    # 3. Pipelined specific stage reports
    if {$is_pipe} {
        catch {
            report_timing -to [get_cells -hierarchical -filter {NAME =~ *douta_pipe_reg*}] \
                -file [file join $out_dir "timing_to_pipe.rpt"] \
                -max_paths 10 -nworst 1 -path_type full_clock_expanded
        }
        catch {
            report_timing -from [get_cells -hierarchical -filter {NAME =~ *douta_pipe_reg*}] \
                -to [get_cells -hierarchical -filter {NAME =~ test_reg*}] \
                -file [file join $out_dir "timing_pipe_to_test.rpt"] \
                -max_paths 10 -nworst 1 -path_type full_clock_expanded
        }
    }

    # 4. Utilization
    report_utilization -file [file join $out_dir "utilization.rpt"]

    close_design
    puts ">>> Finished configuration: $run_name"
}

close_project
puts "INFO: All 12 runs completed."
exit 0
