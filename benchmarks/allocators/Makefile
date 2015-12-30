# You may need to set the following variables:
#
# CXX    - path to C++ compiler
# CC     - path to C compiler
# AR     - path to suitable 'ar' tool
# RANLIB - path to suitable 'ranlib' tool
# BDEWAF - path to 'waf' in BDE Tools (https://github.com/bloomberg/bde-tools)
#
# They may be overridden on the command line, e.g. for gcc 5.1 on 4 cores:
# $ make CXX=g++ CC=gcc BDEWAF=$HOME/bde-tools/bin/waf STDLIB= PARALLEL=-j4

CXX = g++ #clang++-3.6
CC = gcc #clang-3.6
AR = llvm-ar-3.6
RANLIB = llvm-ranlib-3.6
BDEWAF = waf

OPTIM = -O3 -DBSLS_ASSERT_LEVEL_NONE
LTO =
DEBUG =
PARALLEL = 

BSLSRC = ../..
BSL = $(BSLSRC)/groups/bsl
BDL = $(BSLSRC)/groups/bdl
BSLINC = $(INST)/include/bsl
BDLINC = $(INST)/include/bdl
BSLLIB = $(BSLSRC)/build/groups/bsl
BDLLIB = $(BSLSRC)/build/groups/bdl

# comment this out for gcc-5:
# STDLIB = -stdlib=libc++ # TODO: Clang
# If libc++ was built from source (not installed from a system package),
# then libc++abi may not have been linked into libc++. In this situation, the
# linker must be explicitly told to include it in the link, by uncommenting
# the line below.
#STDLIBABI = -lc++abi

INCLUDES = \
   -I$(BSL)/bsls -I$(BSL)/bslma -I$(BSL)/bslscm -I$(BSL)/bslh \
   -I$(BSL)/bsl+bslhdrs -I$(BSL)/bslstl -I$(BSL)/bslmf -I$(BSL)/bslalg \
   -I$(BDL)/bdlscm -I$(BDL)/bdlma

DEFS = -D_REENTRANT -D_POSIX_PTHREAD_SEMANTICS -DBSLS_IDENT_OFF
WAFCONFIGARGS = \
    --abi-bits=64 --cpp11 --build-type release \
#    --assert-level=none $(PARALLEL) 

LIBS = \
   -Wl,-Bstatic \
   -L$(BDLLIB) -lbdl -L$(BSLLIB) -lbsl \
   -Wl,-Bdynamic \
   $(STDLIBABI) \
   -lpthread

CFLAGS_BDE = $(DEBUG) $(OPTIM) $(LTO) $(DEFS) $(CFLAGS) -std=c99
CXXFLAGS_BDE = $(DEBUG) $(OPTIM) $(LTO) $(DEFS) $(STDLIB)
CXXFLAGS_LOCAL = $(CXXFLAGS_BDE) -std=c++14 $(INCLUDES) $(CXXFLAGS)
LDFLAGS_LOCAL = $(LIBS) $(LDFLAGS)

clean:
	rm -rf $(BINARIES) bde-tag $(BSLSRC)/build growth-*-* *-result

reallyclean: clean
	rm -rf $(OUTPUT)

bde-tag: Makefile
	( \
	cd $(BSLSRC) && \
	export CC='$(CC)' && \
	export CXX='$(CXX)' && \
	export AR='$(AR)' && \
	export RANLIB='$(RANLIB)' && \
	export CFLAGS='$(CFLAGS_BDE)' && \
	export CXXFLAGS='$(CXXFLAGS_BDE)' && \
	$(BDEWAF) configure $(WAFCONFIGARGS) && \
	$(BDEWAF) build -v \
	)
	touch bde-tag

benchmark_1: benchmark_1.cc benchmark_common.h bde-tag
	$(CXX) -o $@ $(CXXFLAGS_LOCAL) $< $(LDFLAGS_LOCAL)

benchmark_2: benchmark_2.cc benchmark_common.h bde-tag
	$(CXX) -o $@ $(CXXFLAGS_LOCAL) $< $(LDFLAGS_LOCAL)