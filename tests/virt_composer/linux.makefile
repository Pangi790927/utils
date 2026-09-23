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

# The tool that keeps VIRT_COMPOSER_ABI's hash current. It is not a test - it has a main() and no
# print_test_result - so it is taken out of the wildcard the way the plugins are kept out of it by
# living in a directory of their own. 22-09-2026-12:40
ABI_TOOL     := abi_hash_tool$(EXE_EXT)

TEST_FILES   := $(filter-out abi_hash_tool.cpp,$(wildcard *.cpp))
TEST_TARGETS := $(patsubst %.cpp,%$(EXE_EXT),$(TEST_FILES))

# Plugins live one directory down precisely so this wildcard does not sweep them up: a plugin has
# no main() and is not a test, it is something a test loads. Each is built into a .so of its own.
# 2026-09-20 16:07
PLUGIN_FILES   := $(wildcard plugins/*.cpp)
PLUGIN_TARGETS := $(patsubst %.cpp,%.so,$(PLUGIN_FILES))

# abi-sync runs first, in a make of its own, and the build in a second one. make judges every
# target before it runs any recipe, so a header rewritten during the same make was judged stale
# too late: the plugins kept the old VIRT_COMPOSER_ABI and the five plugin tests failed until the
# next run. The second make judges against the header as abi-sync left it, plugins included.
# 23-09-2026-04:48
.PHONY: all run
all:
	@$(MAKE) --no-print-directory abi-sync
	@$(MAKE) --no-print-directory run

run: $(PLUGIN_TARGETS) $(TEST_TARGETS)
	@python3 run_tests.py $(TEST_TARGETS)

# It carries no copy of virt_composer and must not: it rewrites the header, so it cannot also be
# built from the value it is rewriting. abi_hash.h is everything it needs. 22-09-2026-12:40
$(ABI_TOOL): abi_hash_tool.cpp abi_hash.h
	${CXX} ${CXX_FLAGS} abi_hash_tool.cpp ${CXX_OUT} $@

# Run once per build, and before anything below compiles the header it may rewrite - which is the
# whole point of it being here rather than in the test runner. The test reads its value as a
# compile-time macro, so a value written after compilation is one the test cannot see, and it
# would fail on the very run that repaired it. Order-only, so the rewrite does not itself count as
# a reason to rebuild. 22-09-2026-12:40
.PHONY: abi-sync
abi-sync: $(ABI_TOOL)
	@./$(ABI_TOOL)

$(CORE_OBJ): ../../virt_composer.cpp ../../virt_composer.h ../../virt_object.h | abi-sync
	${CXX} ${CXX_FLAGS} -c ../../virt_composer.cpp ${CXX_OUT} $(CORE_OBJ)

$(TEST_TARGETS): %$(EXE_EXT): %.cpp $(CORE_OBJ) tests_common.h ../../virt_composer.h ../../virt_composer_end.h | abi-sync
	${CXX} ${CXX_FLAGS} $< $(CORE_OBJ) ${CXX_OUT} $@ ${LIBS}

# No CORE_OBJ here, and that is the whole point: a plugin carries no copy of virt_composer and
# takes the host's at load time, so one library state serves the process. The undefined symbols
# that leaves are resolved then, not now. 2026-09-20 16:07
# The one plugin built claiming a virt_composer it was not built against, so that load_plugin's
# version refusal can be reached at all. VIRT_COMPOSER_ABI is guarded in virt_composer.h for this
# and for nothing else - see its doc comment. 2026-09-20 18:45
plugins/mock_plugin_stale.so: CXX_FLAGS += -DVIRT_COMPOSER_ABI='"0.0-stale"'

PLUGIN_DEPS := $(wildcard plugins/*.h) $(wildcard plugins/*/*.h)

# The same order-only prerequisite as everything else, and it matters more here than anywhere: a
# plugin bakes VIRT_COMPOSER_ABI in, and a host that carries a different one refuses to load it.
# A plugin built before the sync and a test built after it disagree, which is the refusal working
# on a mismatch nobody meant. 22-09-2026-12:50
$(PLUGIN_TARGETS): %.so: %.cpp $(PLUGIN_DEPS) ../../virt_composer.h ../../virt_composer_end.h | abi-sync
	${CXX} ${CXX_FLAGS} -fPIC -shared $< ${CXX_OUT} $@

clean:
	rm -f $(TEST_TARGETS)
	rm -f $(PLUGIN_TARGETS)
	rm -f *.o
	rm -f $(ABI_TOOL)
	rm -f *.tmp.yaml
