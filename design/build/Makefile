#a Copyright
#  
#  This file 'Makefile' copyright Embisi Inc., 2004

#a Targets
all:

clean: docclean simclean toolsclean

sim: simulation

doc: documentation

.PHONY: simulation

simulation:
	$(MAKE) -f Makefile.simulation

simclean:
	$(MAKE) -f Makefile.simulation clean

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

