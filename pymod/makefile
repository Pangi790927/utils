NAME      := pymod.so

UTILS     := ../
INCLCUDES := -I${UTILS} -I${UTILS}/ap -I${UTILS}/co -I${UTILS}/generic -I.
INCLCUDES += -I/usr/include/python3.8
LIBS      := -lpthread -ldl -lglfw -lcurl
LIBS      += -lpython3.8

SRCS      := $(wildcard ./*.cpp)
SRCS      += $(wildcard ${UTILS}/*.cpp)
OBJS      := $(SRCS:.cpp=.o)
DEPS      := $(SRCS:.cpp=.d)
CXX 	  := g++-11
CXX_FLAGS := -std=c++2a -g -fPIC -O3 -export-dynamic
CXX_FLAGS += -Wno-format-security
CXX_FLAGS += -DPYMOD_NOINIT_FUNCTION
CXX_FLAGS += -DPYMOD_MODULE_NAME=PyInit_pymod

all: ${NAME}

${NAME}: ${DEPS} ${OBJS}
	${CXX} ${CXX_FLAGS} ${INCLCUDES} -shared ${OBJS} ${LIBS} -o $@

${DEPS}: makefile
${OBJS}: makefile

${DEPS}:%.d:%.cpp
	${CXX} -c ${CXX_FLAGS} ${INCLCUDES} -MM $< -MF $@

include ${DEPS}

${OBJS}:%.o:%.cpp
	${CXX} -c ${CXX_FLAGS} ${INCLCUDES} $< -o $@

clean:
	rm -f ${OBJS}
	rm -f ${DEPS}
	rm -f ${NAME}