#a Copyright
#  
#  This file 'Makefile' copyright Gavin J Stark 2003, 2004
#  

#a Global variables
GIP_PREFIX := ../../../../../common/bin/
GIP_AS := ${GIP_PREFIX}gip_asm

#TEST_LIST := src/test_list
#CREATE_MAKE := scripts/create_make

#ALL2: ALL obj/tests.make
ALL2: ALL

obj/tests.make: ${CREATE_MAKE} ${TEST_LIST}
	${CREATE_MAKE} ${TEST_LIST} obj/tests.make

include obj/tests.make

TEST_BINARIES := bin/gip_iram_0.mif bin/gip_iram_1.mif

ALL: $(TEST_BINARIES)

bin/gip_iram_0.mif: src/gip_iram_0.S
	$(GIP_AS) src/gip_iram_0.S bin/gip_iram_0.mif

bin/gip_iram_1.mif: src/gip_iram_1.S
	$(GIP_AS) src/gip_iram_1.S bin/gip_iram_1.mif

obj/wrapper.o: src/common/wrapper.s
	${ARMCC} -c src/common/wrapper.s -o obj/wrapper.o

clean:
	rm -f obj/* bin/*

#depend:
#	makedepend -- ${CFLAGS} -- ${SRCS}

#a Editor preferences and notes
# Local Variables: ***
# mode: Makefile ***
# outline-regexp: "#[a!]\\\|#[	 ]*[b-z][	 ]" ***
# End: ***

# DO NOT DELETE
