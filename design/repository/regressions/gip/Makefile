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

TEST_BINARIES := bin/test1.gif bin/test_pst.gif

ALL: $(TEST_BINARIES)

bin/test1.gif: src/test1.s
	$(GIP_AS) src/test1.s bin/test1.gif

bin/test_pst.gif: src/test_pst.s
	$(GIP_AS) src/test_pst.s bin/test_pst.gif

bin/test2.gif: src/test2.s
	$(GIP_AS) src/test2.s bin/test2.gif

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
