INCLCUDES := -I../ -I../ap -I../generic -I../vulkan -I.
LIBS      := -lpthread -ldl -lglfw -lcurl -lvulkan
# LIBS      += -lSPIRV -lSPIRV-Tools -lSPIRV-Tools-opt -lglslang -lOGLCompiler -lOSDependent 
# LIBS      += -lSPVRemapper -lpthread -lHLSL

LIBS += -Wl,--start-group
	# https://github.com/google/shaderc
	LIBS += -lglslang -lSPIRV -lOSDependent
	LIBS += -lSPIRV-Tools -lSPIRV-Tools-opt -lSPVRemapper

	SHADERC_ERR := $(shell g++ -lshaderc_shared 2>&1)
	ifneq (,$(findstring cannot find -lshaderc_shared,$(SHADERC_ERR)))
		LIBS += -lOGLCompiler -lHLSL
	else
		LIBS += -lshaderc_shared
	endif

	MACHINE_INDEPENDENT_ERR := $(shell g++ -lMachineIndependent 2>&1)
	ifeq (,$(findstring cannot find -lMachineIndependent,$(MACHINE_INDEPENDENT_ERR)))
		LIBS += -lMachineIndependent -lGenericCodeGen
	endif
LIBS += -Wl,--end-group

LIBS      += -lpthread
LIBS      += -lbacktrace

CXX       := g++

SRCS      := $(wildcard ../*.cpp)
SRCS      += $(wildcard ../ap/*.cpp)
SRCS      += $(wildcard ../vulkan/*.cpp)
OBJS      := $(SRCS:.cpp=.o)
DEPS      := $(SRCS:.cpp=.d)
CXX_FLAGS := -std=c++2a -g -export-dynamic -O3 -fPIE
CXX_FLAGS += -Wall -Wextra -Wno-format-security

# For when I need to debug compilation time
# CXX_FLAGS += -Q
# CXX_FLAGS += -ftime-report

TEST_SRCS := $(wildcard ./*.cpp)
TESTS     := $(TEST_SRCS:%.cpp=%.bin)
TEST_CALLS:= $(TEST_SRCS:%.cpp=%.bin-call)
TEST_OBJS := $(TEST_SRCS:%.cpp=%.o)
DEPS      += $(TEST_SRCS:.cpp=.d)

all: ${TESTS} ${TEST_CALLS}

$(info OBJS: ${OBJS})
$(info DEPS: ${OBJS})

${TESTS}:%.bin:%.o ${DEPS} ${OBJS} 
	${CXX} ${CXX_FLAGS} ${INCLCUDES} ${OBJS} $< ${LIBS} -o $@
	
${TEST_CALLS}:%-call:%
	./$<

${DEPS}: makefile
${OBJS}: makefile

${DEPS}:%.d:%.cpp
	${CXX} -c ${CXX_FLAGS} ${INCLCUDES} -MM $< -MF $@

include ${DEPS}

${OBJS}:%.o:%.cpp
	${CXX} -c ${CXX_FLAGS} ${INCLCUDES} $< -o $@

${TEST_OBJS}:%.o:%.cpp
	${CXX} -c ${CXX_FLAGS} ${INCLCUDES} $< -o $@

clean:
	rm -f ${OBJS}
	rm -f ${DEPS}
	rm -f ${NAME}
	rm -f ${TEST_OBJS}
	rm -f ${TESTS}
	rm -f data/*
