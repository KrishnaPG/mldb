# Note: clang 3.6 won't compile MLDB if GCC6 is installed

CLANGXX?=clang++
CLANG?=clang

CLANG_VERSION:=$(shell $(CLANGXX) --version | head -n1 | awk '{ print $4; }' | sed 's/-*//g')
CXX_VERSION?=$(CLANG_VERSION)
CXX := $(COMPILER_CACHE) $(CLANGXX)
BUILDING_WITH_CLANG:=1

$(warning building with clang++ version $(CLANG_VERSION))

CLANGXXWARNINGFLAGS?=-Wall -Werror -Wno-sign-compare -Woverloaded-virtual -Wno-deprecated-declarations -Wno-deprecated -Winit-self -Qunused-arguments -Wno-mismatched-tags -Wno-unused-function -ftemplate-backtrace-limit=0 -Wno-inconsistent-missing-override

CXXFLAGS ?= $(ARCHFLAGS) $(INCLUDE) $(CLANGXXWARNINGFLAGS) -pipe -ggdb $(foreach dir,$(LOCAL_INCLUDE_DIR),-I$(dir)) -std=c++17 
CXXNODEBUGFLAGS := -O3 -DBOOST_DISABLE_ASSERTS -DNDEBUG 
CXXDEBUGFLAGS := -O0 -g3

CXXLINKFLAGS = -rdynamic $(foreach DIR,$(PWD)/$(BIN) $(PWD)/$(LIB),-L$(DIR) -Wl,--rpath-link,$(DIR)) -Wl,--rpath,\$$ORIGIN/../bin -Wl,--rpath,\$$ORIGIN/../lib -Wl,--copy-dt-needed-entries -Wl,--no-as-needed
CXXLIBRARYFLAGS = -shared $(CXXLINKFLAGS) -lpthread
CXXEXEFLAGS =$(CXXLINKFLAGS) -lpthread
CXXEXEPOSTFLAGS = $(if $(MEMORY_ALLOC_LIBRARY),-l$(MEMORY_ALLOC_LIBRARY))



CC ?= $(COMPILER_CACHE) $(CLANG)
GCCWARNINGFLAGS?=-Wall -Werror -Wno-sign-compare -fno-strict-overflow -Wno-unused-but-set-variable
CFLAGS ?= $(ARCHFLAGS) $(INCLUDE) $(GCCWARNINGFLAGS) -pipe -O3 -g -DNDEBUG -fno-omit-frame-pointer
CDEBUGFLAGS := -O0 -g
CNODEBUGFLAGS := -O3 -g -DNDEBUG 

FC ?= gfortran
FFLAGS ?= $(ARCHFLAGS) -I. -fPIC

# Library name of filesystem
STD_FILESYSTEM_LIBNAME:=stdc++fs
