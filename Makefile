# Configuration
CFLAGS = -Wall -DUSE_POSIX_MEMALIGN
CXXFLAGS = -std=c++14 -Wall -DUSE_POSIX_MEMALIGN
INCLUDES = -Isrc
LDFLAGS = -lz

ifdef USE_METIS
CFLAGS += -DUSE_METIS
CXXFLAGS += -DUSE_METIS
LDFLAGS += -lmetis
endif

ifndef NO_LIBPFM
CFLAGS += -DHAVE_LIBPFM
CXXFLAGS += -DHAVE_LIBPFM
LDFLAGS += -lpfm
endif

ifndef NO_LIBNUMA
CFLAGS += -DHAVE_LIBNUMA
CXXFLAGS += -DHAVE_LIBNUMA
LDFLAGS += -lnuma
endif

ifndef NO_OPENMP
CFLAGS += -fopenmp -DUSE_OPENMP
CXXFLAGS += -fopenmp -DUSE_OPENMP
endif

ifndef NO_GTEST
ifdef GTEST_ROOT
GTEST_INCLUDES = -isystem $(GTEST_ROOT)/include
GTEST_LIBS = -Wl,-rpath,$(GTEST_ROOT)/lib -L$(GTEST_ROOT)/lib
endif
GTEST_LIBS += -lgtest -lgtest_main
endif

ifdef DEBUG
CFLAGS += -O2 -g -fsanitize=address
CXXFLAGS += -O2 -g -fsanitize=address
else
CFLAGS += -O3
CXXFLAGS += -O3
endif

ifeq ($(CXX),icpc)
CXXFLAGS += -D__PURE_INTEL_C99_HEADERS__ -D'__builtin_addressof(x) reinterpret_cast<_Tp*>(&const_cast<char&>(reinterpret_cast<const volatile char&>(x)));'
endif

ifdef USE_INTEL_MKL
MKL_INCLUDES = -isystem ${MKLROOT}/include
MKL_LIBS = \
	-L${MKLROOT}/lib/intel64 \
	-Wl,--no-as-needed \
	-lmkl_intel_lp64 \
	-lmkl_gnu_thread \
	-lmkl_core \
	-liomp5 \
	-lpthread \
	-lm \
	-ldl
CFLAGS += -DUSE_INTEL_MKL
CXXFLAGS += -DUSE_INTEL_MKL
INCLUDES += $(MKL_INCLUDES)
LDFLAGS += $(MKL_LIBS)
endif

# Default
.PHONY: all
all: spmv-cache-trace


# Libraries
util_a = src/util/util.a
util_c_sources = \
	src/util/json.c
util_c_headers = \
	src/util/json.h
util_c_objects := \
	$(foreach source,$(util_c_sources),$(source:.c=.o))
util_cxx_sources = \
	src/util/indenting-ostreambuf.cpp \
	src/util/json-ostreambuf.cpp \
	src/util/perf-events.cpp \
	src/util/tarstream.cpp \
	src/util/zlibstream.cpp
util_cxx_headers = \
	src/util/indenting-ostreambuf.hpp \
	src/util/circular-buffer.hpp \
	src/util/json-ostreambuf.hpp \
	src/util/perf-events.hpp \
	src/util/tarstream.hpp \
	src/util/zlibstream.hpp
util_cxx_objects := \
	$(foreach source,$(util_cxx_sources),$(source:.cpp=.o))

$(util_c_objects): %.o: %.c $(util_c_headers)
	$(CC) -c $(CFLAGS) $(INCLUDES) $< -o $@
$(util_cxx_objects): %.o: %.cpp $(util_cxx_headers)
	$(CXX) -c $(CXXFLAGS) $(INCLUDES) $< -o $@
$(util_a): $(util_c_objects) $(util_cxx_objects)
	$(AR) $(ARFLAGS) $@ $^

# Matrix
matrix_a = src/matrix/matrix.a
matrix_sources = \
	src/matrix/coo-matrix.cpp \
	src/matrix/csr-matrix.cpp \
	src/matrix/csr-matrix-spmv.cpp \
	src/matrix/ell-matrix.cpp \
	src/matrix/hybrid-matrix.cpp \
	src/matrix/matrix-error.cpp \
	src/matrix/matrix-market.cpp \
	src/matrix/matrix-market-reorder.cpp
matrix_headers = \
	src/matrix/coo-matrix.hpp \
	src/matrix/csr-matrix.hpp \
	src/matrix/ell-matrix.hpp \
	src/matrix/hybrid-matrix.hpp \
	src/matrix/matrix-error.hpp \
	src/matrix/matrix-market.hpp \
	src/matrix/matrix-market-reorder.hpp
matrix_objects := \
	$(foreach source,$(matrix_sources),$(source:.cpp=.o))

$(matrix_objects): %.o: %.cpp $(matrix_headers)
	$(CXX) -c $(CXXFLAGS) $(INCLUDES) $< -o $@
$(matrix_a): $(matrix_objects)
	$(AR) $(ARFLAGS) $@ $^

cache_simulation_a = src/cache-simulation/cache-simulation.a
cache_simulation_sources = \
	src/cache-simulation/fifo.cpp \
	src/cache-simulation/lru.cpp \
	src/cache-simulation/rand.cpp \
	src/cache-simulation/replacement.cpp
cache_simulation_headers = \
	src/cache-simulation/replacement.hpp
cache_simulation_objects := \
	$(foreach source,$(cache_simulation_sources),$(source:.cpp=.o))

$(cache_simulation_objects): %.o: %.cpp $(cache_simulation_headers)
	$(CXX) -c $(CXXFLAGS) $(INCLUDES) $< -o $@
$(cache_simulation_a): $(cache_simulation_objects)
	$(AR) $(ARFLAGS) $@ $^

# Kernels
kernels_a = src/cache-simulation/kernels.a
kernels_sources = \
	src/kernels/coo-spmv.cpp \
	src/kernels/coo-spmv-atomic.cpp \
	src/kernels/csr-spmv.cpp \
	src/kernels/ell-spmv.cpp \
	src/kernels/mkl-csr-spmv.cpp \
	src/kernels/hybrid-spmv.cpp \
	src/kernels/triad.cpp \
	src/kernels/kernel.cpp
kernels_headers = \
	src/kernels/coo-spmv.hpp \
	src/kernels/coo-spmv-atomic.hpp \
	src/kernels/csr-spmv.hpp \
	src/kernels/ell-spmv.hpp \
	src/kernels/mkl-csr-spmv.hpp \
	src/kernels/hybrid-spmv.hpp \
	src/kernels/triad.hpp \
	src/kernels/kernel.hpp
kernels_objects := \
	$(foreach source,$(kernels_sources),$(source:.cpp=.o))

$(kernels_objects): %.o: %.cpp $(kernels_headers)
	$(CXX) -c $(CXXFLAGS) $(INCLUDES) $< -o $@
$(kernels_a): $(kernels_objects)
	$(AR) $(ARFLAGS) $@ $^

# Main
spmv_cache_trace_sources = \
	src/cache-trace.cpp \
	src/main.cpp \
	src/profile-kernel.cpp \
	src/trace-config.cpp
spmv_cache_trace_headers = \
	src/cache-trace.hpp \
	src/kernels.hpp \
	src/profile-kernel.hpp \
	src/trace-config.hpp
spmv_cache_trace_objects := \
	$(foreach source,$(spmv_cache_trace_sources),$(source:.cpp=.o))

$(spmv_cache_trace_objects): %.o: %.cpp $(spmv_cache_trace_headers)
	$(CXX) -c $(CXXFLAGS) $(INCLUDES) $< -o $@
spmv-cache-trace: $(spmv_cache_trace_objects) $(cache_simulation_a) $(kernels_a) $(matrix_a) $(util_a)
	$(CXX) $(CXXFLAGS) $(INCLUDES) $^ $(LDFLAGS) -o $@


# Testing
.PHONY: check
check: unittest
	./unittest

# Build the unit tests
unittest_sources = \
	test/test_aligned-allocator.cpp \
	test/test_circular-buffer.cpp \
	test/test_json.cpp \
	test/test_json_ostreambuf.cpp \
	test/test_matrix-market.cpp \
	test/test_coo-matrix.cpp \
	test/test_csr-matrix.cpp \
	test/test_ell-matrix.cpp \
	test/test_hybrid-matrix.cpp \
	test/test_perf-events.cpp \
	test/test_replacement.cpp \
	test/test_sample.cpp
unittest_objects := \
	$(foreach source,$(unittest_sources),$(source:.cpp=.o))

$(unittest_objects): %.o: %.cpp
	$(CXX) $(CPPFLAGS) -c $(CXXFLAGS) $(INCLUDES) $(GTEST_INCLUDES) $^ -o $@
unittest: $(unittest_objects) $(cache_simulation_a) $(kernels_a) $(matrix_a) $(util_a)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDFLAGS) $(GTEST_LIBS) -o $@


# Clean
.PHONY: clean
clean:
	rm -f $(util_c_objects) $(util_cxx_objects) $(util_a)
	rm -f $(matrix_objects) $(matrix_a)
	rm -f $(cache_simulation_objects) $(cache_simulation_a)
	rm -f $(kernels_objects) $(kernels_a)
	rm -f $(spmv_cache_trace_objects)
	rm -f spmv-cache-trace
	rm -f $(unittest_objects)
	rm -f unittest
