set rtl_dir [file dirname [file normalize [info script]]]
set prj_dir "$rtl_dir/prj"
set src_dir "$rtl_dir/src"
set include_dir "$src_dir/include"
set pipeline_dir "$src_dir/pipeline"
set part xc7a35tcpg236-1

create_project -force pipeline $prj_dir -part $part

add_files -fileset sources_1 -norecurse \
    $src_dir/lib/lulea_cache_rom.sv     \
    $include_dir/lulea_config.svh       \
    $include_dir/lulea_types.svh        \
    $pipeline_dir/l1.sv                 \
    $pipeline_dir/l2.sv                 \
    $pipeline_dir/l3.sv                 \
    $pipeline_dir/pipeline.sv

add_files -fileset sources_1 -norecurse [glob "$rtl_dir/mem/*.mem"]

add_files -fileset constrs_1 -norecurse "$rtl_dir/constraints/rv_nsu_basys_3.xdc"

set_property include_dirs [list $include_dir] [get_filesets sources_1]
set_property top pipeline [get_filesets sources_1]
update_compile_order -fileset sources_1

synth_design -top pipeline -part $part -mode out_of_context
write_checkpoint -force "$prj_dir/post_synth.dcp"
report_utilization -file "$prj_dir/utilization.rpt"
