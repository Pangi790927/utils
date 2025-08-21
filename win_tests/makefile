# Some usefull line arguments:
# /EHsc 								- eable exceptions
# /D<name>{=|#}<text> 					- define macro
# /I<dir> 								- add to include search path
# /std:<c++14|c++17|c++20|c++latest> 	- C++ standard version
# /await 								- enable resumable functions extension
# /Zc:preprocessor						- sane preproc
# /Zc:__cplusplus,enumTypes				- other sane enables
# /Zc:templateScope,threadSafeInit,auto - other sane enables
# /c 									- compile only, no link
# /V<string>							- set version string
# /utf-8 								- set source and execution character set to UTF-8
# /LD 									- Create .DLL
# /MT vs /MD							- static vs dynamic something runtime
# /external:I <path>      				- location of external headers
# /Zi									- enable debug info

# C:\'Program Files (x86)\Windows Kits'\10\Debuggers\x64\cdb.exe -o main.exe
# list of commands: ?
# list of commands: https://learn.microsoft.com/en-us/windows-hardware/drivers/debuggercmds/commands

CXX := cl
CXX_FLAGS := /EHsc /await:strict /std:c++20 /Zi
INCLUDES := /I..

all: test_colib test_mmcb

test_colib:
	${CXX} ${CXX_FLAGS} ${INCLUDES} test_colib.cpp
	./test_colib.exe

test_mmcb:	
	${CXX} ${CXX_FLAGS} ${INCLUDES} test_mmcb.cpp Onecore.lib
	./test_mmcb.exe
