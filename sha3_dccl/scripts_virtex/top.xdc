create_clock -period 7.000 -name ap_clk -waveform {0.000 3.500} [get_ports ap_clk]
set_input_delay -clock [get_clocks ap_clk] -min -add_delay 0.100 [get_ports {dataPort_V_datain[*]}]
set_input_delay -clock [get_clocks ap_clk] -max -add_delay 0.100 [get_ports {dataPort_V_datain[*]}]
set_input_delay -clock [get_clocks ap_clk] -min -add_delay 0.100 [get_ports ap_rst]
set_input_delay -clock [get_clocks ap_clk] -max -add_delay 0.100 [get_ports ap_rst]
set_input_delay -clock [get_clocks ap_clk] -min -add_delay 0.100 [get_ports ap_start]
set_input_delay -clock [get_clocks ap_clk] -max -add_delay 0.100 [get_ports ap_start]
set_input_delay -clock [get_clocks ap_clk] -min -add_delay 0.100 [get_ports dataPort_V_req_full_n]
set_input_delay -clock [get_clocks ap_clk] -max -add_delay 0.100 [get_ports dataPort_V_req_full_n]
set_input_delay -clock [get_clocks ap_clk] -min -add_delay 0.100 [get_ports dataPort_V_rsp_empty_n]
set_input_delay -clock [get_clocks ap_clk] -max -add_delay 0.100 [get_ports dataPort_V_rsp_empty_n]
set_output_delay -clock [get_clocks ap_clk] -min -add_delay 0.100 [get_ports {dataPort_V_address[*]}]
set_output_delay -clock [get_clocks ap_clk] -max -add_delay 0.100 [get_ports {dataPort_V_address[*]}]
set_output_delay -clock [get_clocks ap_clk] -min -add_delay 0.100 [get_ports {dataPort_V_dataout[*]}]
set_output_delay -clock [get_clocks ap_clk] -max -add_delay 0.100 [get_ports {dataPort_V_dataout[*]}]
set_output_delay -clock [get_clocks ap_clk] -min -add_delay 0.100 [get_ports {dataPort_V_size[*]}]
set_output_delay -clock [get_clocks ap_clk] -max -add_delay 0.100 [get_ports {dataPort_V_size[*]}]
set_output_delay -clock [get_clocks ap_clk] -min -add_delay 0.100 [get_ports ap_done]
set_output_delay -clock [get_clocks ap_clk] -max -add_delay 0.100 [get_ports ap_done]
set_output_delay -clock [get_clocks ap_clk] -min -add_delay 0.100 [get_ports ap_idle]
set_output_delay -clock [get_clocks ap_clk] -max -add_delay 0.100 [get_ports ap_idle]
set_output_delay -clock [get_clocks ap_clk] -min -add_delay 0.100 [get_ports ap_ready]
set_output_delay -clock [get_clocks ap_clk] -max -add_delay 0.100 [get_ports ap_ready]
set_output_delay -clock [get_clocks ap_clk] -min -add_delay 0.100 [get_ports dataPort_V_req_din]
set_output_delay -clock [get_clocks ap_clk] -max -add_delay 0.100 [get_ports dataPort_V_req_din]
set_output_delay -clock [get_clocks ap_clk] -min -add_delay 0.100 [get_ports dataPort_V_req_write]
set_output_delay -clock [get_clocks ap_clk] -max -add_delay 0.100 [get_ports dataPort_V_req_write]
set_output_delay -clock [get_clocks ap_clk] -min -add_delay 0.100 [get_ports dataPort_V_rsp_read]
set_output_delay -clock [get_clocks ap_clk] -max -add_delay 0.100 [get_ports dataPort_V_rsp_read]
