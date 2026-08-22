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
CXX_FLAGS  := /nologo /EHs /await:strict /std:c++20 /Zc:preprocessor /Zi /I..\..
CXX_OUT    := /Fe:
LINK_FLAGS := /link
EXE_EXT    := .exe

# virt_composer.cpp (not just virt_composer.h) must be compiled and linked into every test binary
# - it's not header-only, it's where create_state()/parse_config()/the Lua bridge/etc. are
# actually defined (see virt_composer.h's own top-of-file docs). Building it once here and
# relinking it into every test .exe avoids recompiling ~1100 lines of Lua-bridge templates once
# per test file.
CORE_OBJ   := virt_composer_core.obj

TEST_FILES   := $(wildcard *.cpp)
TEST_TARGETS := $(patsubst %.cpp,%$(EXE_EXT),$(TEST_FILES))

# 'all' runs every test regardless of earlier failures, same rationale (and same run_tests.py) as
# co-lib/tests/windows.makefile - cmd.exe's errorlevel handling across a `for` loop is unreliable
# enough that driving the pass/fail summary from Python is simpler than getting it right in Make.
all: $(TEST_TARGETS)
	@python run_tests.py $(TEST_TARGETS)

$(CORE_OBJ): ..\..\virt_composer.cpp ..\..\virt_composer.h ..\..\virt_object.h
	${CXX} ${CXX_FLAGS} /c ..\..\virt_composer.cpp /Fo:$(CORE_OBJ)

# Every test target depends on tests_common.h and virt_composer.h/_end.h (not just its own .cpp)
# so editing any of those correctly invalidates every test's stale .exe on the next `make`.
$(TEST_TARGETS): %$(EXE_EXT): %.cpp $(CORE_OBJ) tests_common.h ..\..\virt_composer.h ..\..\virt_composer_end.h
	${CXX} ${CXX_FLAGS} $< $(CORE_OBJ) ${CXX_OUT}$@ ${LINK_FLAGS}

clean:
	-del /F /Q *.exe 2>nul
	-del /F /Q *.obj 2>nul
	-del /F /Q *.pdb 2>nul
	-del /F /Q *.ilk 2>nul
	-del /F /Q *.tmp.yaml 2>nul
