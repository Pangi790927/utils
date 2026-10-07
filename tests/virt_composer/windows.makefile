# Pinning SHELL to cmd.exe (always present on any Windows install, unlike a POSIX shell, which
# needs Git-for-Windows/MSYS's usr/bin on PATH) means every recipe line below runs through a real
# shell instead of GNU Make trying (and failing) to CreateProcess the first word directly. `cl`
# itself still needs a Developer Command Prompt / vcvars environment on PATH - that's a compiler
# requirement no makefile can paper over. (Mirrors ../../../co-lib/tests/windows.makefile.)
SHELL       := cmd.exe
.SHELLFLAGS := /c

CXX        := cl
# /Zc:preprocessor is not optional: virt_composer.h/.cpp use debug.h's DBG()/ASSERT_FN() macros,
# which rely on the GCC `, ##__VA_ARGS__` comma-elision extension when called with zero variadic
# arguments (e.g. `DBG("literal")`). MSVC's legacy/"traditional" preprocessor (the default even
# under /std:c++20) mangles that pattern into invalid tokens - see e.g. virt_composer.cpp's
# ASSERT_RET(nullptr, ...) call sites. The conformant preprocessor handles it correctly.
# NOMINMAX is defined by every test file itself (see tests_common.h) before anything pulls in
# windows.h, so it isn't repeated here.
# /MD, the C runtime as a DLL: a plugin carries its own copy of the library, but what it allocates
# -- an object, a std::function it registers -- the host's state later frees, so every module must
# allocate from one heap, which only the shared runtime gives. Every module below is built with it.
# 28-09-2026-12:00
CXX_FLAGS  := /nologo /EHs /await:strict /std:c++20 /Zc:preprocessor /Zi /MD /I..\..
CXX_OUT    := /Fe:
# A test shows vc_host_name_owner, the one thing a plugin must reach in its host (see VC_API in
# virt_composer.h), and a plugin shows its entry points; neither is linked against by anything, so
# no import library or export file is made for them. 28-09-2026-12:00
LINK_FLAGS := /link /NOIMPLIB /NOEXP
EXE_EXT    := .exe

# virt_composer.cpp (not just virt_composer.h) must be compiled and linked into every test binary
# - it's not header-only, it's where create_state()/parse_config()/the Lua bridge/etc. are
# actually defined. Built once here and linked into every test and every plugin: each module
# carries its own copy of the library. 28-09-2026-12:00
CORE_OBJ   := virt_composer_core.obj

# The plugin component, which every module links beside CORE_OBJ, a test and a plugin alike, as
# in linux.makefile; on Windows one object serves both. 06-10-2026-02:45
PLUGINS_OBJ := virt_composer_plugins_core.obj

# The coroutine component, which every module links too, so a plugin's member may answer a
# co::task; on Windows one object serves a test and a plugin alike. 06-10-2026-06:34
CORO_OBJ := virt_composer_coroutines_core.obj

# The tool that keeps VIRT_COMPOSER_ABI's hash current, as in linux.makefile. It is not a test.
# 27-09-2026-09:40
ABI_TOOL     := abi_hash_tool$(EXE_EXT)

TEST_FILES   := $(filter-out abi_hash_tool.cpp,$(wildcard *.cpp))
TEST_TARGETS := $(patsubst %.cpp,%$(EXE_EXT),$(TEST_FILES))

# Plugins live one directory down so the wildcard above does not sweep them up, as in
# linux.makefile. Each is a DLL of its own. 27-09-2026-09:40
PLUGIN_FILES   := $(wildcard plugins/*.cpp)
PLUGIN_TARGETS := $(patsubst %.cpp,%.dll,$(PLUGIN_FILES))

# abi-sync runs in a make of its own before the build, for the reason linux.makefile gives: make
# judges every target before it runs any recipe. 'run' runs every test regardless of earlier
# failures, same rationale (and same run_tests.py) as co-lib/tests/windows.makefile - cmd.exe's
# errorlevel handling across a `for` loop is unreliable enough that driving the pass/fail summary
# from Python is simpler than getting it right in Make. 27-09-2026-09:40
.PHONY: all run abi-sync
all:
	@$(MAKE) --no-print-directory abi-sync
	@$(MAKE) --no-print-directory run

run: $(PLUGIN_TARGETS) $(TEST_TARGETS)
	@python run_tests.py $(TEST_TARGETS)

$(ABI_TOOL): abi_hash_tool.cpp abi_hash.h
	${CXX} ${CXX_FLAGS} abi_hash_tool.cpp ${CXX_OUT}$@

abi-sync: $(ABI_TOOL)
	@.\$(ABI_TOOL)

$(CORE_OBJ): ..\..\virt_composer.cpp ..\..\virt_composer.h ..\..\virt_composer_internal.h \
		..\..\virt_object.h | abi-sync
	${CXX} ${CXX_FLAGS} /c ..\..\virt_composer.cpp /Fo:$(CORE_OBJ)

# Every test target depends on tests_common.h and virt_composer.h/_end.h (not just its own .cpp)
# so editing any of those correctly invalidates every test's stale .exe on the next `make`.
$(PLUGINS_OBJ): ..\..\virt_composer_plugins.cpp ..\..\virt_composer_plugins.h \
		..\..\virt_composer_internal.h ..\..\virt_composer.h ..\..\virt_object.h | abi-sync
	${CXX} ${CXX_FLAGS} /c ..\..\virt_composer_plugins.cpp /Fo:$(PLUGINS_OBJ)

$(CORO_OBJ): ..\..\virt_composer_coroutines.cpp ..\..\virt_composer_coroutines.h \
		..\..\virt_composer_internal.h ..\..\virt_composer.h ..\..\virt_object.h | abi-sync
	${CXX} ${CXX_FLAGS} /c ..\..\virt_composer_coroutines.cpp /Fo:$(CORO_OBJ)

$(TEST_TARGETS): %$(EXE_EXT): %.cpp $(CORE_OBJ) $(PLUGINS_OBJ) $(CORO_OBJ) tests_common.h \
		..\..\virt_composer.h \
		..\..\virt_composer_plugins.h ..\..\virt_composer_end.h | abi-sync
	${CXX} ${CXX_FLAGS} $< $(CORE_OBJ) $(PLUGINS_OBJ) $(CORO_OBJ) ${CXX_OUT}$@ ${LINK_FLAGS}

# The one plugin built claiming a virt_composer it was not built against, as in linux.makefile.
# 27-09-2026-09:40
plugins/mock_plugin_stale.dll: CXX_FLAGS += /DVIRT_COMPOSER_ABI=\"0.0-stale\"

PLUGIN_DEPS := $(wildcard plugins/*.h) $(wildcard plugins/*/*.h)

# A plugin links its own copy of the library, the same object the tests link. /Fo and /Fd keep
# each plugin's object and debug file beside it, so two plugins never write one another's.
# 28-09-2026-12:00
$(PLUGIN_TARGETS): %.dll: %.cpp $(CORE_OBJ) $(PLUGINS_OBJ) $(CORO_OBJ) $(PLUGIN_DEPS) \
		..\..\virt_composer.h \
		..\..\virt_composer_plugins.h ..\..\virt_composer_end.h | abi-sync
	${CXX} ${CXX_FLAGS} /LD $< $(CORE_OBJ) $(PLUGINS_OBJ) $(CORO_OBJ) /Fo:$(basename $@).obj \
		/Fd:$(basename $@).pdb ${CXX_OUT}$@ ${LINK_FLAGS}

clean:
	-del /F /Q *.exe 2>nul
	-del /F /Q *.dll 2>nul
	-del /F /Q *.lib 2>nul
	-del /F /Q *.exp 2>nul
	-del /F /Q *.obj 2>nul
	-del /F /Q *.pdb 2>nul
	-del /F /Q *.ilk 2>nul
	-del /F /Q *.tmp.yaml 2>nul
	-del /F /Q *.tmp.log 2>nul
	-del /F /Q plugins\*.dll plugins\*.lib plugins\*.exp plugins\*.obj plugins\*.pdb 2>nul
