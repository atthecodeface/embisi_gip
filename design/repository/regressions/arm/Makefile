#a Copyright
#  
#  This file 'Makefile' copyright Gavin J Stark 2003, 2004
#  

#a Global variables
ARM_PREFIX := /usr/local/cross_arm/bin/arm-linux-
ARM_CC := ${ARM_PREFIX}gcc
ARM_LD := ${ARM_PREFIX}ld
ARM_STRIP := ${ARM_PREFIX}strip
ARM_CC_FLAGS := -fomit-frame-pointer -O3
#ARM_CC_FLAGS := 

TEST_LIST := src/test_list
CREATE_MAKE := scripts/create_make

ALL2: ALL obj/tests.make

obj/tests.make: ${CREATE_MAKE} ${TEST_LIST}
	${CREATE_MAKE} ${TEST_LIST} obj/tests.make

include obj/tests.make

ALL: $(TEST_BINARIES)

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
