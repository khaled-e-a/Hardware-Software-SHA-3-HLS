
# Get the DIR argument
set DIR [lindex $argv 0]

# Create a project targeting the zedboard
create_project sha3_var_rate -force $DIR/system_zedboard/sha3_var_rate -part xc7z020clg484-1
set_property board_part em.avnet.com:zed:part0:1.2 [current_project]

# Include the IPs into the repository

set_property ip_repo_paths  {../hls_dir/sha3_var_rate_zedboard/solution1 } [current_project]


update_ip_catalog -rebuild

# source the system builder
set ::DIR_global $DIR
source $DIR/scripts_zedboard/build_system.tcl 
unset ::DIR_global