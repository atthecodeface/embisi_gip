export XIL_PLACE_ALLOW_LOCAL_BUFG_ROUTING := 1

REPOSITORY = ../../repository/emulation/gip_system
MACROS = ${REPOSITORY}/macros
XSRC = ${REPOSITORY}/src
RAMS = ${REPOSITORY}/rams
CONSTRAINTS = ${REPOSITORY}/constraints
SRC = ../../build/emulation/linux

SRCS = ${MACROS}/brw.vhd \
		${MACROS}/boot_rom.vhd \
		${MACROS}/rf_1r_1w_16_32.v \
		${MACROS}/rf_1r_1w_16_4.v \
		${MACROS}/rf_1r_1w_32_32.v \
		${MACROS}/rf_2r_1w_32_32.v \
		${MACROS}/memory_s_sp_2048_x_8.v \
		${MACROS}/memory_s_sp_2048_x_32.v \
		${MACROS}/memory_s_sp_4096_x_8.v \
		${MACROS}/memory_s_sp_4096_x_4b8.v \
		${MACROS}/memory_s_dp_2048_x_32.v \
		${MACROS}/boot_rom_wrapper.v \
		${XSRC}/fpga_wrapper.v \
		${XSRC}/clock_generator.v \
		${SRC}/gip_rf.v \
		${SRC}/gip_alu_barrel_shift.v \
		${SRC}/gip_alu_logical_op.v \
		${SRC}/gip_alu_arith_op.v \
		${SRC}/gip_alu.v \
		${SRC}/gip_memory.v \
		${SRC}/gip_scheduler.v \
		${SRC}/gip_decode_arm.v \
		${SRC}/gip_decode_native.v \
		${SRC}/gip_decode.v \
		${SRC}/gip_postbus.v \
		${SRC}/gip_special.v \
		${SRC}/gip_core.v \
		${SRC}/gip_prefetch.v \
		${SRC}/gip_data_ram.v \
		${SRC}/gip_periph_apb.v \
		${SRC}/gip_core_plus.v \
		${SRC}/ddr_dram_as_sram.v \
		${SRC}/apb_uart_sync_fifo.v \
		${SRC}/apb_target_uart.v \
		${SRC}/apb_target_ext_bus_master.v \
		${SRC}/apb_target_gpio.v \
		${SRC}/apb_target_timer.v \
		${SRC}/io_uart.v \
		${SRC}/apb_devices.v \
		${SRC}/io_ingress_control.v \
		${SRC}/io_ingress_fifos.v \
		${SRC}/io_cmd_fifo_timer.v \
		${SRC}/io_egress_control_cmd_fsm.v \
		${SRC}/io_egress_control.v \
		${SRC}/io_egress_fifos.v \
		${SRC}/io_baud_rate_generator.v \
		${SRC}/io_postbus_target.v \
		${SRC}/io_postbus_event_manager.v \
		${SRC}/io_postbus_source.v \
		${SRC}/io_postbus.v \
		${SRC}/io_block.v \
		${SRC}/io_ethernet_tx.v \
		${SRC}/io_ethernet_rx.v \
		${SRC}/io_slot_head.v \
		${SRC}/io_sync_serial.v \
		${SRC}/io_parallel.v \
		${SRC}/io_slots_eth_ss_par.v \
		${SRC}/analyzer.v \
		${SRC}/gip_system.v

XST_OPTIONS = -p xc4vlx60-12-ff668
XST_OPTIONS += -opt_mode Speed
XST_OPTIONS += -opt_level 1
XST_OPTIONS += -glob_opt AllClockNets
XST_OPTIONS += -rtlview No
XST_OPTIONS += -read_cores YES
XST_OPTIONS += -write_timing_constraints NO
XST_OPTIONS += -cross_clock_analysis NO
XST_OPTIONS += -hierarchy_separator /
XST_OPTIONS += -bus_delimiter <>
XST_OPTIONS += -case maintain
XST_OPTIONS += -slice_utilization_ratio 100
XST_OPTIONS += -verilog2001 YES
XST_OPTIONS += -fsm_extract YES
XST_OPTIONS += -fsm_encoding Auto
XST_OPTIONS += -safe_implementation No
XST_OPTIONS += -fsm_style lut
XST_OPTIONS += -ram_extract Yes
XST_OPTIONS += -ram_style Auto
XST_OPTIONS += -rom_extract Yes
XST_OPTIONS += -rom_style Auto
XST_OPTIONS += -mux_extract YES
XST_OPTIONS += -mux_style Auto
XST_OPTIONS += -decoder_extract YES
XST_OPTIONS += -priority_extract YES
XST_OPTIONS += -shreg_extract YES
XST_OPTIONS += -shift_extract YES
XST_OPTIONS += -xor_collapse YES
XST_OPTIONS += -resource_sharing YES
XST_OPTIONS += -use_dsp48 auto
XST_OPTIONS += -max_fanout 500
XST_OPTIONS += -bufg 32
XST_OPTIONS += -bufr Default
XST_OPTIONS += -register_duplication YES
XST_OPTIONS += -equivalent_register_removal YES
XST_OPTIONS += -register_balancing No
XST_OPTIONS += -slice_packing YES
XST_OPTIONS += -optimize_primitives NO
XST_OPTIONS += -use_clock_enable Auto
XST_OPTIONS += -use_sync_set Auto
XST_OPTIONS += -use_sync_reset Auto
XST_OPTIONS += -enable_auto_floorplanning No
XST_OPTIONS += -iob auto
XST_OPTIONS += -slice_utilization_ratio_maxmargin 5

XST_COMP_OPTIONS = ${XST_OPTIONS} -iobuf no -ofmt ngc
# -keep-hierarchy NO


all:
	@echo "Make synth, or make ngd, or make map, or make par, or make trace, or make bitgen"

obj/xst/gip_core.ngc: ${SRC}/gip_core.v ${SRC}/gip_rf.v ${SRC}/gip_alu.v ${SRC}/gip_memory.v ${SRC}/gip_postbus.v ${SRC}/gip_special.v ${SRC}/gip_scheduler.v ${SRC}/gip_decode.v
	mkdir -p obj/xst
	mkdir -p tmp/xst
	rm -f obj/xst/.built
	echo "verilog  work ${SRC}/gip_core.v"          > tmp/xst/gip_core.prj
	echo "verilog  work ${SRC}/gip_rf.v"           >> tmp/xst/gip_core.prj
	echo "verilog  work ${SRC}/gip_alu.v"          >> tmp/xst/gip_core.prj
	echo "verilog  work ${SRC}/gip_memory.v"       >> tmp/xst/gip_core.prj
	echo "verilog  work ${SRC}/gip_postbus.v"      >> tmp/xst/gip_core.prj
	echo "verilog  work ${SRC}/gip_special.v"      >> tmp/xst/gip_core.prj
	echo "verilog  work ${SRC}/gip_scheduler.v"    >> tmp/xst/gip_core.prj
	echo "verilog  work ${SRC}/gip_decode.v"       >> tmp/xst/gip_core.prj
	echo "set -tmpdir ./tmp"                        > tmp/xst/gip_core.xst
	echo "set -xsthdpdir ./xst"                    >> tmp/xst/gip_core.xst
	echo "run"                                     >> tmp/xst/gip_core.xst
	echo "${XST_COMP_OPTIONS}  -ifmt mixed -ifn tmp/xst/gip_core.prj -ofn ./obj/xst/gip_core -top gip_core" >> tmp/xst/gip_core.xst
	xst -ifn tmp/xst/gip_core.xst -ofn log/gip_core.syr

obj/xst/gip_core_plus.ngc: obj/xst/gip_core.ngc ${SRC}/gip_core_plus.v
	mkdir -p obj/xst
	mkdir -p tmp/xst
	rm -f obj/xst/.built
	echo "MODEL \"gip_core\" incremental_synthesis=yes;" > tmp/xst/gip_core_plus.xcf
	echo "verilog  work ${SRC}/gip_core_plus.v"     > tmp/xst/gip_core_plus.prj
	echo "verilog  work ${SRC}/gip_prefetch.v"     >> tmp/xst/gip_core_plus.prj
	echo "verilog  work ${SRC}/gip_data_ram.v"     >> tmp/xst/gip_core_plus.prj
	echo "verilog  work ${SRC}/gip_periph_apb.v"   >> tmp/xst/gip_core_plus.prj
	echo "set -tmpdir ./tmp"                        > tmp/xst/gip_core_plus.xst
	echo "set -xsthdpdir ./xst"                    >> tmp/xst/gip_core_plus.xst
	echo "run"                                     >> tmp/xst/gip_core_plus.xst
	echo "-uc tmp/xst/gip_core_plus.xcf"          >> tmp/xst/gip_core_plus.xst
	echo "${XST_COMP_OPTIONS}  -ifmt mixed -ifn tmp/xst/gip_core_plus.prj -ofn ./obj/xst/gip_core_plus -top gip_core_plus" >> tmp/xst/gip_core_plus.xst
	xst -ifn tmp/xst/gip_core_plus.xst -ofn log/gip_core_plus.syr

obj/xst/ddr_dram_as_sram.ngc: ${SRC}/ddr_dram_as_sram.v
	mkdir -p obj/xst
	mkdir -p tmp/xst
	rm -f obj/xst/.built
	echo "verilog  work ${SRC}/ddr_dram_as_sram.v"  > tmp/xst/ddr_dram_as_sram.prj
	echo "set -tmpdir ./tmp"                        > tmp/xst/ddr_dram_as_sram.xst
	echo "set -xsthdpdir ./xst"                    >> tmp/xst/ddr_dram_as_sram.xst
	echo "run"                                     >> tmp/xst/ddr_dram_as_sram.xst
	echo "${XST_COMP_OPTIONS}  -ifmt mixed -ifn tmp/xst/ddr_dram_as_sram.prj -ofn ./obj/xst/ddr_dram_as_sram -top ddr_dram_as_sram" >> tmp/xst/ddr_dram_as_sram.xst
	xst -ifn tmp/xst/ddr_dram_as_sram.xst -ofn log/ddr_dram_as_sram.syr

obj/xst/io_postbus.ngc: build/io_postbus.xst ${SRC}/io_postbus.v ${SRC}/io_postbus_target.v ${SRC}/io_postbus_source.v ${SRC}/io_postbus_event_manager.v
	mkdir -p obj/xst
	rm -f obj/xst/.built
	xst -ifn build/io_postbus.xst -ofn log/io_postbus.syr

obj/xst/.built: ${SRCS} build/fpga_wrapper.xst obj/xst/io_postbus.ngc
	xst -ifn build/fpga_wrapper.xst -ofn log/fpga_wrapper.syr
	touch obj/xst/.built

synth: obj/xst/.built
obj/xst/fpga_wrapper.ngc: obj/xst/.built

obj/coregen/boot_rom.edn: build/boot_rom.xco ${RAMS}/boot.coe
	mkdir -p obj/coregen
	rm -f obj/coregen/boot_rom*
	coregen -b build/boot_rom.xco

obj/xst/boot_rom.ngo: obj/coregen/boot_rom.edn
	mkdir -p obj/xst
	edif2ngd obj/coregen/boot_rom.edn obj/xst/boot_rom.ngo

coregen: obj/xst/boot_rom.ngo


obj/ngd/.built: obj/xst/boot_rom.ngo obj/xst/.built ${CONSTRAINTS}/fpga_wrapper_lx60.ucf obj/xst/fpga_wrapper.ngc
	mkdir -p obj/ngd
	rm -f obj/ngd/.built
	ngdbuild -dd ./tmp -nt timestamp -uc ${CONSTRAINTS}/fpga_wrapper_lx60.ucf -p xc4vlx60-ff668-12 obj/xst/fpga_wrapper.ngc obj/ngd/fpga_wrapper.ngd
	touch obj/ngd/.built

obj/ngd/fpga_wrapper.ngd: obj/ngd/.built
ngd: obj/ngd/.built


obj/map/.built: obj/ngd/.built obj/ngd/fpga_wrapper.ngd
	mkdir -p obj/map
	rm -f obj/map/.built
	map -p xc4vlx60-ff668-12 -cm area -detail -pr b -k 4 -c 100 -o obj/map/fpga_wrapper.ncd obj/ngd/fpga_wrapper.ngd obj/map/fpga_wrapper.pcf
	touch obj/map/.built

obj/map/fpga_wrapper.ncd: obj/map/.built
obj/map/fpga_wrapper.pcf: obj/map/.built
map: obj/map/.built


obj/par/.built: obj/map/.built obj/map/fpga_wrapper.ncd obj/map/fpga_wrapper.pcf
	mkdir -p obj/par
	rm -f obj/par/.built
	par -w -ol med -t 1 obj/map/fpga_wrapper.ncd obj/par/fpga_wrapper.ncd obj/map/fpga_wrapper.pcf
	touch obj/par/.built

obj/par/fpga_wrapper.pcf: obj/par/.built
par: obj/par/.built


obj/trace/.built: obj/par/.built obj/par/fpga_wrapper.ncd obj/map/fpga_wrapper.pcf
	mkdir -p obj/trace
	rm -f obj/trace/.built
	trce -e 3 -l 3 -s 12 -xml fpga_wrapper obj/par/fpga_wrapper.ncd -o obj/trace/fpga_wrapper.twr obj/map/fpga_wrapper.pcf
	touch obj/trace/.built

trace: obj/trace/.built


obj/bitgen/.built: obj/par/.built obj/par/fpga_wrapper.ncd obj/map/fpga_wrapper.pcf
	mkdir -p obj/bitgen
	rm -f obj/bitgen/.built
	bitgen -f build/fpga_wrapper.ut obj/par/fpga_wrapper.ncd obj/bitgen/fpga_wrapper.bit obj/map/fpga_wrapper.pcf
	touch obj/bitgen/.built

bitgen: obj/bitgen/.built


clean:
	rm -f obj/* obj/*/.built obj/*/* tmp/* tmp/*/* coregen.log core.[0-9]*

