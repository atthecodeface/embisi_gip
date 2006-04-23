#a Copyright
#  
#  This file 'Makefile' copyright Embisi Inc., 2004

#a Targets
all:

clean: docclean simclean toolsclean

sim: simulation

em: emulation

doc: documentation

.PHONY: simulation

simulation:
	$(MAKE) -f Makefile.simulation

simclean:
	$(MAKE) -f Makefile.simulation clean

.PHONY: emulation

emulation:
	$(MAKE) -f Makefile.emulation verilog

emclean:
	$(MAKE) -f Makefile.emulation clean

.PHONY: xilinx

xilinx:
	$(MAKE) -f Makefile.xilinx all

xilinxclean:
	$(MAKE) -f Makefile.xilinx clean

.PHONY: documentation

documentation:
	$(MAKE) -f Makefile.documentation

docclean:
	rm -rf documentation

.PHONY: tools
tools:
	$(MAKE) -f Makefile.tools

toolsclean:
	$(MAKE) -f Makefile.tools clean

#a Editor preferences and notes
# Local Variables: ***
# mode: Makefile ***
# outline-regexp: "#[a!]\\\|#[\t ]*[b-z][\t ]" ***
# End: ***

