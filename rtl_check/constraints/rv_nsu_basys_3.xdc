#-------------------------------------------------------------------------------
#    cfg:           BASYS-3
#
#    description:   
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
#    configuration options
#-------------------------------------------------------------------------------
set_property CFGBVS VCCO [current_design]
set_property CONFIG_VOLTAGE 3.3 [current_design]

#-------------------------------------------------------------------------------
#    clocks
#-------------------------------------------------------------------------------

set_property PACKAGE_PIN W5 [get_ports ref_clk]
set_property IOSTANDARD LVCMOS33 [get_ports ref_clk]
create_clock -add -name ref_clk -period 10.0  [get_ports ref_clk]
#create_clock -name ref_clk -period 10.0  [get_ports ref_clk]
#set_switching_activity -deassert_resets
