INCLCUDES := -I../ -I../ap -I../generic -I.
LIBS      := -lpthread -ldl -lglfw -lcurl

SRCS      := $(wildcard ../*.cpp)
SRCS      += $(wildcard ../ap/*.cpp)
OBJS      := $(SRCS:.cpp=.o)
DEPS      := $(SRCS:.cpp=.d)
CXX_FLAGS := -std=c++2a -g -export-dynamic -O3
CXX_FLAGS += -Wno-format-security

TEST_SRCS := $(wildcard ./*.cpp)
TESTS     := $(TEST_SRCS:%.cpp=%.bin)
TEST_CALLS:= $(TEST_SRCS:%.cpp=%.bin-call)
TEST_OBJS := $(TEST_SRCS:%.cpp=%.o)
DEPS      += $(TEST_SRCS:.cpp=.d)

all: ${TESTS} ${TEST_CALLS}

$(info OBJS: ${OBJS})
$(info DEPS: ${OBJS})

${TESTS}:%.bin:%.o ${DEPS} ${OBJS} 
	g++ ${CXX_FLAGS} ${INCLCUDES} ${OBJS} $< ${LIBS} -o $@
	
${TEST_CALLS}:%-call:%
	./$<

${DEPS}: makefile
${OBJS}: makefile

${DEPS}:%.d:%.cpp
	g++ -c ${CXX_FLAGS} ${INCLCUDES} -MM $< -MF $@

include ${DEPS}

${OBJS}:%.o:%.cpp
	g++ -c ${CXX_FLAGS} ${INCLCUDES} $< -o $@

${TEST_OBJS}:%.o:%.cpp
	g++ -c ${CXX_FLAGS} ${INCLCUDES} $< -o $@

clean:
	rm -f ${OBJS}
	rm -f ${DEPS}
	rm -f ${NAME}
	rm -f ${TESTS}
	rm -f data/*
