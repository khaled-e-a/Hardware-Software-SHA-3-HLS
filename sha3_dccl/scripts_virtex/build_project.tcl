
# Get the DIR argument
set DIR [lindex $argv 0]

# Create a project targeting the virtex
create_project sha3_var_rate -force $DIR/system_virtex/sha3_var_rate -part xc7z020clg484-1
set_property board_part xilinx.com:vc709:part0:1.5 [current_project]
# Include the IPs into the repository

set_property ip_repo_paths  {../hls_dir/sha3_var_rate_virtex/solution1 } [current_project]


update_ip_catalog -rebuild

# source the system builder
set ::DIR_global $DIR
source $DIR/scripts_virtex/build_system.tcl 
unset ::DIR_global