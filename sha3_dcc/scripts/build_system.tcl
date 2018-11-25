set $DIR $::DIR_global

update_ip_catalog -rebuild

create_bd_design "design_1"


# Place and configure the zynq

create_bd_cell -type ip -vlnv xilinx.com:ip:processing_system7:5.5 processing_system7_0

apply_bd_automation -rule xilinx.com:bd_rule:processing_system7 -config {make_external "FIXED_IO, DDR" apply_board_preset "1" Master "Disable" Slave "Disable" }  [get_bd_cells processing_system7_0]

set_property -dict [list CONFIG.PCW_QSPI_PERIPHERAL_ENABLE {0} CONFIG.PCW_ENET0_PERIPHERAL_ENABLE {0} CONFIG.PCW_SD0_PERIPHERAL_ENABLE {0} CONFIG.PCW_TTC0_PERIPHERAL_ENABLE {0} CONFIG.PCW_USB0_PERIPHERAL_ENABLE {0} CONFIG.PCW_GPIO_MIO_GPIO_ENABLE {0}] [get_bd_cells processing_system7_0]


# Place AXI HS 64 bit port

set_property -dict [list CONFIG.PCW_USE_S_AXI_HP0 {1}] [get_bd_cells processing_system7_0]


#place keccak 

create_bd_cell -type ip -vlnv xilinx.com:hls:sha3_256:1.0 sha3_256_0

apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {Master "/sha3_256_0/m_axi_dataPort_V" Clk "Auto" }  [get_bd_intf_pins processing_system7_0/S_AXI_HP0]

# Place GPIO 

create_bd_cell -type ip -vlnv xilinx.com:ip:axi_gpio:2.0 axi_gpio_0


apply_bd_automation -rule xilinx.com:bd_rule:axi4 -config {Master "/processing_system7_0/M_AXI_GP0" Clk "Auto" }  [get_bd_intf_pins axi_gpio_0/S_AXI]

set_property -dict [list CONFIG.C_GPIO_WIDTH {1} CONFIG.C_GPIO2_WIDTH {1} CONFIG.C_IS_DUAL {1} CONFIG.C_ALL_INPUTS {1} CONFIG.C_ALL_OUTPUTS_2 {1}] [get_bd_cells axi_gpio_0]




connect_bd_net [get_bd_pins axi_gpio_0/gpio2_io_o] [get_bd_pins sha3_256_0/ap_start]

connect_bd_net [get_bd_pins sha3_256_0/ap_idle] [get_bd_pins axi_gpio_0/gpio_io_i]



create_bd_cell -type ip -vlnv xilinx.com:ip:ila:5.0 ila_0

set_property location {3.5 935 79} [get_bd_cells ila_0]
connect_bd_net [get_bd_pins sha3_256_0/m_axi_dataPort_V_AWADDR] [get_bd_pins axi_mem_intercon/S00_AXI_awaddr]

connect_bd_net [get_bd_pins sha3_256_0/m_axi_dataPort_V_AWVALID] [get_bd_pins axi_mem_intercon/S00_AXI_awvalid]

connect_bd_net [get_bd_pins sha3_256_0/m_axi_dataPort_V_AWREADY] [get_bd_pins axi_mem_intercon/S00_AXI_awready]

connect_bd_net [get_bd_pins sha3_256_0/m_axi_dataPort_V_WDATA] [get_bd_pins axi_mem_intercon/S00_AXI_wdata]

connect_bd_net [get_bd_pins sha3_256_0/m_axi_dataPort_V_WVALID] [get_bd_pins axi_mem_intercon/S00_AXI_wvalid]

connect_bd_net [get_bd_pins sha3_256_0/m_axi_dataPort_V_WREADY] [get_bd_pins axi_mem_intercon/S00_AXI_wready]

connect_bd_net [get_bd_pins sha3_256_0/m_axi_dataPort_V_ARADDR] [get_bd_pins axi_mem_intercon/S00_AXI_araddr]

connect_bd_net [get_bd_pins sha3_256_0/m_axi_dataPort_V_ARVALID] [get_bd_pins axi_mem_intercon/S00_AXI_arvalid]

connect_bd_net [get_bd_pins sha3_256_0/m_axi_dataPort_V_ARREADY] [get_bd_pins axi_mem_intercon/S00_AXI_arready]

connect_bd_net [get_bd_pins sha3_256_0/m_axi_dataPort_V_RDATA] [get_bd_pins axi_mem_intercon/S00_AXI_rdata]

connect_bd_net [get_bd_pins sha3_256_0/m_axi_dataPort_V_RVALID] [get_bd_pins axi_mem_intercon/S00_AXI_rvalid]

connect_bd_net [get_bd_pins sha3_256_0/m_axi_dataPort_V_RREADY] [get_bd_pins axi_mem_intercon/S00_AXI_rready]


set_property -dict [list CONFIG.C_PROBE3_WIDTH {64} CONFIG.C_PROBE2_WIDTH {64} CONFIG.C_PROBE1_WIDTH {32} CONFIG.C_PROBE0_WIDTH {32} CONFIG.C_NUM_OF_PROBES {14} CONFIG.C_MONITOR_TYPE {Native} CONFIG.C_ENABLE_ILA_AXI_MON {false}] [get_bd_cells ila_0]

connect_bd_net -net [get_bd_nets sha3_256_0_m_axi_dataPort_V_AWADDR] [get_bd_pins ila_0/probe0] [get_bd_pins sha3_256_0/m_axi_dataPort_V_AWADDR]
connect_bd_net -net [get_bd_nets sha3_256_0_m_axi_dataPort_V_WDATA] [get_bd_pins ila_0/probe2] [get_bd_pins sha3_256_0/m_axi_dataPort_V_WDATA]
connect_bd_net -net [get_bd_nets sha3_256_0_m_axi_dataPort_V_ARADDR] [get_bd_pins ila_0/probe1] [get_bd_pins sha3_256_0/m_axi_dataPort_V_ARADDR]
connect_bd_net -net [get_bd_nets axi_mem_intercon_S00_AXI_rdata] [get_bd_pins ila_0/probe3] [get_bd_pins axi_mem_intercon/S00_AXI_rdata]
connect_bd_net -net [get_bd_nets sha3_256_0_m_axi_dataPort_V_AWVALID] [get_bd_pins ila_0/probe4] [get_bd_pins sha3_256_0/m_axi_dataPort_V_AWVALID]
connect_bd_net -net [get_bd_nets axi_mem_intercon_S00_AXI_awready] [get_bd_pins ila_0/probe5] [get_bd_pins axi_mem_intercon/S00_AXI_awready]
connect_bd_net -net [get_bd_nets sha3_256_0_m_axi_dataPort_V_WVALID] [get_bd_pins ila_0/probe6] [get_bd_pins sha3_256_0/m_axi_dataPort_V_WVALID]
connect_bd_net -net [get_bd_nets axi_mem_intercon_S00_AXI_wready] [get_bd_pins ila_0/probe7] [get_bd_pins axi_mem_intercon/S00_AXI_wready]
connect_bd_net -net [get_bd_nets sha3_256_0_m_axi_dataPort_V_ARVALID] [get_bd_pins ila_0/probe8] [get_bd_pins sha3_256_0/m_axi_dataPort_V_ARVALID]
connect_bd_net -net [get_bd_nets axi_mem_intercon_S00_AXI_arready] [get_bd_pins ila_0/probe9] [get_bd_pins axi_mem_intercon/S00_AXI_arready]
connect_bd_net -net [get_bd_nets axi_mem_intercon_S00_AXI_rvalid] [get_bd_pins ila_0/probe10] [get_bd_pins axi_mem_intercon/S00_AXI_rvalid]
connect_bd_net -net [get_bd_nets sha3_256_0_m_axi_dataPort_V_RREADY] [get_bd_pins ila_0/probe11] [get_bd_pins sha3_256_0/m_axi_dataPort_V_RREADY]
connect_bd_net -net [get_bd_nets processing_system7_0_FCLK_CLK0] [get_bd_pins ila_0/clk] [get_bd_pins processing_system7_0/FCLK_CLK0]
set_property location {2 892 270} [get_bd_cells axi_gpio_0]
connect_bd_net -net [get_bd_nets sha3_256_0_ap_idle] [get_bd_pins ila_0/probe12] [get_bd_pins sha3_256_0/ap_idle]
connect_bd_net -net [get_bd_nets axi_gpio_0_gpio2_io_o] [get_bd_pins ila_0/probe13] [get_bd_pins axi_gpio_0/gpio2_io_o]
set_property name ap_start [get_bd_nets axi_gpio_0_gpio2_io_o]
set_property name ap_idle [get_bd_nets sha3_256_0_ap_idle]




validate_bd_design





reset_target all [get_files  $DIR/system/sha3_pipe/sha3_pipe.srcs/sources_1/bd/design_1/design_1.bd]
generate_target all [get_files  $DIR/system/sha3_pipe/sha3_pipe.srcs/sources_1/bd/design_1/design_1.bd]
make_wrapper -files [get_files $DIR/system/sha3_pipe/sha3_pipe.srcs/sources_1/bd/design_1/design_1.bd] -top
add_files -norecurse $DIR/system/sha3_pipe/sha3_pipe.srcs/sources_1/bd/design_1/hdl/design_1_wrapper.v
update_compile_order -fileset sources_1
update_compile_order -fileset sim_1
save_bd_design

reset_run synth_1
launch_runs impl_1 -to_step write_bitstream

open_run impl_1

launch_runs impl_1 -to_step write_bitstream