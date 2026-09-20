CXX        := g++
CXX_FLAGS  := -std=c++2a -O0 -g -Wno-format-security -I../..
CXX_OUT    := -o
EXE_EXT    := .bin
# -export-dynamic puts the test binary's own symbols in the dynamic table, which is how a loaded
# plugin reaches virt_composer: the plugin links against nothing and resolves every one of those
# symbols from whoever loaded it. Without it a plugin opens with undefined symbols. It costs the
# other tests nothing, so it is set once here rather than for one target. 2026-09-20 16:07
LIBS       := -lpthread -ldl -export-dynamic

# virt_composer.cpp (not just virt_composer.h) must be compiled and linked into every test binary
# - it's not header-only. Built once here and relinked into every test .bin, same rationale as
# windows.makefile's CORE_OBJ.
CORE_OBJ   := virt_composer_core.o

TEST_FILES   := $(wildcard *.cpp)
TEST_TARGETS := $(patsubst %.cpp,%$(EXE_EXT),$(TEST_FILES))

# Plugins live one directory down precisely so this wildcard does not sweep them up: a plugin has
# no main() and is not a test, it is something a test loads. Each is built into a .so of its own.
# 2026-09-20 16:07
PLUGIN_FILES   := $(wildcard plugins/*.cpp)
PLUGIN_TARGETS := $(patsubst %.cpp,%.so,$(PLUGIN_FILES))

all: $(PLUGIN_TARGETS) $(TEST_TARGETS)
	@python3 run_tests.py $(TEST_TARGETS)

$(CORE_OBJ): ../../virt_composer.cpp ../../virt_composer.h ../../virt_object.h
	${CXX} ${CXX_FLAGS} -c ../../virt_composer.cpp ${CXX_OUT} $(CORE_OBJ)

$(TEST_TARGETS): %$(EXE_EXT): %.cpp $(CORE_OBJ) tests_common.h ../../virt_composer.h ../../virt_composer_end.h
	${CXX} ${CXX_FLAGS} $< $(CORE_OBJ) ${CXX_OUT} $@ ${LIBS}

# No CORE_OBJ here, and that is the whole point: a plugin carries no copy of virt_composer and
# takes the host's at load time, so one library state serves the process. The undefined symbols
# that leaves are resolved then, not now. 2026-09-20 16:07
# The one plugin built claiming a virt_composer it was not built against, so that load_plugin's
# version refusal can be reached at all. VIRT_COMPOSER_ABI is guarded in virt_composer.h for this
# and for nothing else - see its doc comment. 2026-09-20 18:45
plugins/mock_plugin_stale.so: CXX_FLAGS += -DVIRT_COMPOSER_ABI='"0.0-stale"'

PLUGIN_DEPS := $(wildcard plugins/*.h) $(wildcard plugins/*/*.h)

$(PLUGIN_TARGETS): %.so: %.cpp $(PLUGIN_DEPS) ../../virt_composer.h ../../virt_composer_end.h
	${CXX} ${CXX_FLAGS} -fPIC -shared $< ${CXX_OUT} $@

clean:
	rm -f $(TEST_TARGETS)
	rm -f $(PLUGIN_TARGETS)
	rm -f *.o
	rm -f *.tmp.yaml
