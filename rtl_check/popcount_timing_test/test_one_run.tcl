set script_dir [file normalize [file dirname [info script]]]
set proj_dir   [file normalize [file join $script_dir ".." "prj1"]]
set results_dir [file join $script_dir "results"]

file mkdir $results_dir

open_project [file join $proj_dir "prj1.xpr"]

set run_name "run_01_bram16_nopipe_100mhz"
set out_dir [file join $results_dir $run_name]
file mkdir $out_dir

set_property verilog_define [list USE_BRAM_16 USE_CLK_100] [get_filesets sources_1]

reset_run synth_1
launch_runs synth_1 -jobs 8
wait_on_run synth_1

puts "synth_1 status: [get_property STATUS [get_runs synth_1]]"

reset_run impl_1
launch_runs impl_1 -jobs 8
wait_on_run impl_1

puts "impl_1 status: [get_property STATUS [get_runs impl_1]]"

open_run impl_1

report_timing_summary -file [file join $out_dir "timing_summary.rpt"] -max_paths 10
report_timing -to [get_cells -hierarchical -filter {NAME =~ test_reg*}] \
    -file [file join $out_dir "timing_to_test.rpt"] \
    -max_paths 10 -nworst 1 -path_type full_clock_expanded
report_utilization -file [file join $out_dir "utilization.rpt"]

close_design
close_project
puts "SUCCESS: $run_name finished"
exit 0
