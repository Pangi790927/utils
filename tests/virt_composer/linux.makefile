CXX        := g++
CXX_FLAGS  := -std=c++2a -O0 -g -Wno-format-security -I../..
CXX_OUT    := -o
EXE_EXT    := .bin
LIBS       := -lpthread -ldl

# virt_composer.cpp (not just virt_composer.h) must be compiled and linked into every test binary
# - it's not header-only. Built once here and relinked into every test .bin, same rationale as
# windows.makefile's CORE_OBJ.
CORE_OBJ   := virt_composer_core.o

TEST_FILES   := $(wildcard *.cpp)
TEST_TARGETS := $(patsubst %.cpp,%$(EXE_EXT),$(TEST_FILES))

all: $(TEST_TARGETS)
	@python3 run_tests.py $(TEST_TARGETS)

$(CORE_OBJ): ../../virt_composer.cpp ../../virt_composer.h ../../virt_object.h
	${CXX} ${CXX_FLAGS} -c ../../virt_composer.cpp ${CXX_OUT} $(CORE_OBJ)

$(TEST_TARGETS): %$(EXE_EXT): %.cpp $(CORE_OBJ) tests_common.h ../../virt_composer.h ../../virt_composer_end.h
	${CXX} ${CXX_FLAGS} $< $(CORE_OBJ) ${CXX_OUT} $@ ${LIBS}

clean:
	rm -f $(TEST_TARGETS)
	rm -f *.o
	rm -f *.tmp.yaml
