CXX        := g++
CXX_FLAGS  := -std=c++2a -O0 -g -Wno-format-security -I../..
CXX_OUT    := -o
EXE_EXT    := .bin
LIBS       := -lpthread -ldl

# Every module carries its own copy of virt_composer: each test links it, and each plugin links a
# copy of its own. A test is a host, and shows the one symbol a plugin must reach in its host,
# vc_host_name_owner, and nothing else: see VC_API in virt_composer.h. 28-09-2026-12:00
HOST_LINK  := -Wl,--export-dynamic-symbol=vc_host_name_owner

# virt_composer.cpp (not just virt_composer.h) must be compiled and linked into every test binary
# - it's not header-only. Built once here and relinked into every test .bin, same rationale as
# windows.makefile's CORE_OBJ.
CORE_OBJ   := virt_composer_core.o

# The plugin component, virt_composer_plugins.cpp, which every module links beside
# virt_composer.cpp: a test this copy, built as CORE_OBJ is, and a plugin PLUGIN_PLUGINS_OBJ, built
# as PLUGIN_OBJ is. 06-10-2026-02:45
CORE_PLUGINS_OBJ := virt_composer_plugins_core.o

# The coroutine component, virt_composer_coroutines.cpp, which every module links too, so a
# plugin's member may answer a co::task: a test this copy, a plugin PLUGIN_CORO_OBJ. 06-10-2026-06:34
CORE_CORO_OBJ := virt_composer_coroutines_core.o

# A plugin's copy is built apart: position-independent, and hidden, so that nothing of it but what
# the plugin marks for export is seen from outside, and nothing of it is confused with another
# module's. 28-09-2026-12:00
PLUGIN_OBJ   := virt_composer_plugin.o
PLUGIN_PLUGINS_OBJ := virt_composer_plugins_plugin.o
PLUGIN_CORO_OBJ := virt_composer_coroutines_plugin.o
PLUGIN_FLAGS := -fPIC -fvisibility=hidden -fvisibility-inlines-hidden

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

$(CORE_OBJ): ../../virt_composer.cpp ../../virt_composer.h ../../virt_composer_internal.h \
		../../virt_object.h | abi-sync
	${CXX} ${CXX_FLAGS} -c ../../virt_composer.cpp ${CXX_OUT} $(CORE_OBJ)

$(CORE_PLUGINS_OBJ): ../../virt_composer_plugins.cpp ../../virt_composer_plugins.h \
		../../virt_composer_internal.h ../../virt_composer.h ../../virt_object.h | abi-sync
	${CXX} ${CXX_FLAGS} -c ../../virt_composer_plugins.cpp ${CXX_OUT} $(CORE_PLUGINS_OBJ)

$(PLUGIN_PLUGINS_OBJ): ../../virt_composer_plugins.cpp ../../virt_composer_plugins.h \
		../../virt_composer_internal.h ../../virt_composer.h ../../virt_object.h | abi-sync
	${CXX} ${CXX_FLAGS} ${PLUGIN_FLAGS} -c ../../virt_composer_plugins.cpp ${CXX_OUT} $(PLUGIN_PLUGINS_OBJ)

$(CORE_CORO_OBJ): ../../virt_composer_coroutines.cpp ../../virt_composer_coroutines.h \
		../../virt_composer_internal.h ../../virt_composer.h ../../virt_object.h | abi-sync
	${CXX} ${CXX_FLAGS} -c ../../virt_composer_coroutines.cpp ${CXX_OUT} $(CORE_CORO_OBJ)

$(PLUGIN_CORO_OBJ): ../../virt_composer_coroutines.cpp ../../virt_composer_coroutines.h \
		../../virt_composer_internal.h ../../virt_composer.h ../../virt_object.h | abi-sync
	${CXX} ${CXX_FLAGS} ${PLUGIN_FLAGS} -c ../../virt_composer_coroutines.cpp ${CXX_OUT} \
		$(PLUGIN_CORO_OBJ)

$(PLUGIN_OBJ): ../../virt_composer.cpp ../../virt_composer.h ../../virt_composer_internal.h \
		../../virt_object.h | abi-sync
	${CXX} ${CXX_FLAGS} ${PLUGIN_FLAGS} -c ../../virt_composer.cpp ${CXX_OUT} $(PLUGIN_OBJ)

$(TEST_TARGETS): %$(EXE_EXT): %.cpp $(CORE_OBJ) $(CORE_PLUGINS_OBJ) $(CORE_CORO_OBJ) tests_common.h \
		../../virt_composer.h \
		../../virt_composer_plugins.h ../../virt_composer_end.h | abi-sync
	${CXX} ${CXX_FLAGS} $< $(CORE_OBJ) $(CORE_PLUGINS_OBJ) $(CORE_CORO_OBJ) ${CXX_OUT} $@ ${LIBS} \
		${HOST_LINK}

# The one plugin built claiming a virt_composer it was not built against, so that load_plugin's
# version refusal can be reached at all. VIRT_COMPOSER_ABI is guarded in virt_composer.h for this
# and for nothing else - see its doc comment. 2026-09-20 18:45
plugins/mock_plugin_stale.so: CXX_FLAGS += -DVIRT_COMPOSER_ABI='"0.0-stale"'

PLUGIN_DEPS := $(wildcard plugins/*.h) $(wildcard plugins/*/*.h)

# The same order-only prerequisite as everything else, and it matters more here than anywhere: a
# plugin bakes VIRT_COMPOSER_ABI in, and a host that carries a different one refuses to load it.
# A plugin built before the sync and a test built after it disagree, which is the refusal working
# on a mismatch nobody meant. 22-09-2026-12:50
$(PLUGIN_TARGETS): %.so: %.cpp $(PLUGIN_OBJ) $(PLUGIN_PLUGINS_OBJ) $(PLUGIN_CORO_OBJ) $(PLUGIN_DEPS) \
		../../virt_composer.h \
		../../virt_composer_plugins.h ../../virt_composer_end.h | abi-sync
	${CXX} ${CXX_FLAGS} ${PLUGIN_FLAGS} -shared $< $(PLUGIN_OBJ) $(PLUGIN_PLUGINS_OBJ) \
		$(PLUGIN_CORO_OBJ) ${CXX_OUT} $@ ${LIBS}

clean:
	rm -f $(TEST_TARGETS)
	rm -f $(PLUGIN_TARGETS)
	rm -f *.o
	rm -f $(ABI_TOOL)
	rm -f *.tmp.yaml
	rm -f *.tmp.log *.tmp.old.log
