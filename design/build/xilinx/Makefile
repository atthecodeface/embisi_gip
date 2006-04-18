export XIL_PLACE_ALLOW_LOCAL_BUFG_ROUTING := 1

all:
	@echo "Make synth, or make ngdbuild, or make map, or make par, or make trace, or make bitgen"

synth:
	xst -ifn build/fpga_wrapper.xst -ofn log/fpga_wrapper.syr

ngdbuild:
	ngdbuild -dd ./tmp -nt timestamp -uc constraints/fpga_wrapper_lx60.ucf -p xc4vlx60-ff668-12 obj/xst/fpga_wrapper.ngc obj/ngd/fpga_wrapper.ngd

map:
	map -p xc4vlx60-ff668-12 -cm area -detail -pr b -k 4 -c 100 -o obj/map/fpga_wrapper_map.ncd obj/ngd/fpga_wrapper.ngd obj/map/fpga_wrapper.pcf

par:
	par -w -ol med -t 1 fpga_wrapper_map.ncd fpga_wrapper.ncd fpga_wrapper.pcf

trace:
	trce -e 3 -l 3 -s 12 -xml fpga_wrapper fpga_wrapper.ncd -o fpga_wrapper.twr fpga_wrapper.pcf

bitgen:
	bitgen -f build/fpga_wrapper.ut fpga_wrapper.ncd
