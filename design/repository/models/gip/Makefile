OS_DIR := linux
ifeq ($(arch),cygwin)
    OS_DIR := cygwin
endif
objs_from_srcs = $(foreach SRC,${1},$(patsubst %.cpp,${OS_DIR}/%.o,${SRC:.y=.cpp}))

PROG = arm
SRCS = arm_dis.cpp c_gip_pipeline_single.cpp wrapper.cpp c_memory_model.cpp c_ram_model.cpp c_mmio_model.cpp symbols.cpp gdb_stub.cpp console.cpp ether.cpp microkernel.cpp syscalls.cpp
OBJS = $(call objs_from_srcs,${SRCS})

DEBUG_FLAGS = -g
CC = g++ 
LINK = g++
CFLAGS = ${DEBUG_FLAGS} -Wall
CXXFLAGS = ${DEBUG_FLAGS} -Wall
INCLUDES = -I/usr/include/tcl -I/usr/X11R6/include/X11 -I/usr/include
NLIBS = -L/usr/X11R6/lib -ltk -ltcl -lX11 -ldl -lm
LIBS= -lutil
LINKFLAGS = ${DEBUG_FLAGS}
# -shared -export-dynamic

#a Default patterns
${OS_DIR}/%.o : %.cpp
	$(CC) -c ${CXXFLAGS} $< -o $@

${PROG}:	${OBJS}
	${LINK} ${OBJS} -o ${PROG} ${LINKFLAGS} ${LIBS}

arm.exe: arm_dis.o arm_model_test.o c_gip_pipeline_single.o
	g++ -o arm.exe arm_dis.o arm_model_test.o arm_model.o

dis.exe: arm_dis.o arm_dis_test.o arm_model.o
	g++ -o dis.exe arm_dis.o arm_dis_test.o

clean:
	rm -f ${PROG} ${OBJS}

backup:
	tar cvf backup.tar *.cpp *.h to_do *.txt Makefile
	gzip backup.tar
	( filename=`date +%S%M%H%d%m%y`; \
	mv backup.tar.gz backups/bkp$${filename}.tgz; \
	rm -f backups/${PROG}.tgz; \
	ln -s bkp$${filename}.tgz backups/${PROG}.tgz; \
	)

depend: ${SRCS} Makefile
	touch make_depend
	makedepend -f make_depend -- ${CFLAGS} -- $(SRCS)

make_depend: Makefile
	touch make_depend
	makedepend -f make_depend -- ${CFLAGS} -- $(SRCS)

include make_depend
