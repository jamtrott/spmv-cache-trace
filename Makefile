# Configuration
CFLAGS = -Wall -DUSE_POSIX_MEMALIGN
CXXFLAGS = -std=c++14 -Wall -DUSE_POSIX_MEMALIGN
INCLUDES = -Isrc
LDFLAGS = -lz

ifdef USE_LIBPFM
CFLAGS += -DHAVE_LIBPFM
CXXFLAGS += -DHAVE_LIBPFM
LDFLAGS += -lpfm
endif

ifdef USE_LIBNUMA
CFLAGS += -DHAVE_LIBNUMA
CXXFLAGS += -DHAVE_LIBNUMA
LDFLAGS += -lnuma
endif

ifndef NO_OPENMP
CFLAGS += -fopenmp -DUSE_OPENMP
CXXFLAGS += -fopenmp -DUSE_OPENMP
endif

ifdef DEBUG
CFLAGS += -O2 -g -fsanitize=address
CXXFLAGS += -O2 -g -fsanitize=address
else
CFLAGS += -O3 -march=native
CXXFLAGS += -O3 -march=native
endif

ifeq ($(CXX),icpc)
CXXFLAGS += -Iinclude -D__PURE_INTEL_C99_HEADERS__
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
	src/util/json-ostreambuf.cpp \
	src/util/perf-events.cpp \
	src/util/tarstream.cpp \
	src/util/zlibstream.cpp
util_cxx_headers = \
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
	src/matrix/ellpack-matrix.cpp \
	src/matrix/matrix-error.cpp \
	src/matrix/matrix-market.cpp
matrix_headers = \
	src/matrix/coo-matrix.hpp \
	src/matrix/csr-matrix.hpp \
	src/matrix/ellpack-matrix.hpp \
	src/matrix/matrix-error.hpp \
	src/matrix/matrix-market.hpp
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
	src/kernels/csr-spmv.cpp \
	src/kernels/ellpack-spmv.cpp \
	src/kernels/mkl-csr-spmv.cpp \
	src/kernels/triad.cpp \
	src/kernels/kernel.cpp
kernels_headers = \
	src/kernels/coo-spmv.hpp \
	src/kernels/csr-spmv.hpp \
	src/kernels/ellpack-spmv.hpp \
	src/kernels/mkl-csr-spmv.hpp \
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

# Build the unit testing framework
gtest_headers = $(GTEST_ROOT)/include/gtest/*.h \
		$(GTEST_ROOT)/include/gtest/internal/*.h
gtest_sources = $(GTEST_ROOT)/src/*.cc \
		$(GTEST_ROOT)/src/*.h \
		$(gtest_headers)

gtest-all.o: $(gtest_sources)
	$(CXX) $(CPPFLAGS) -I$(GTEST_ROOT) -I$(GTEST_ROOT)/include $(CXXFLAGS) -c \
		$(GTEST_ROOT)/src/gtest-all.cc
gtest_main.o: $(gtest_sources)
	$(CXX) $(CPPFLAGS) -I$(GTEST_ROOT) -I$(GTEST_ROOT)/include $(CXXFLAGS) -c \
		$(GTEST_ROOT)/src/gtest_main.cc
gtest_main.a: gtest-all.o gtest_main.o
	$(AR) $(ARFLAGS) $@ $^

# Build the unit tests
unittest_sources = \
	test/test_aligned-allocator.cpp \
	test/test_circular-buffer.cpp \
	test/test_json.cpp \
	test/test_matrix-market.cpp \
	test/test_coo-matrix.cpp \
	test/test_csr-matrix.cpp \
	test/test_ellpack-matrix.cpp \
	test/test_perf-events.cpp \
	test/test_replacement.cpp \
	test/test_sample.cpp
unittest_objects := \
	$(foreach source,$(unittest_sources),$(source:.cpp=.o))

$(unittest_objects): %.o: %.cpp
	$(CXX) $(CPPFLAGS) -c $(CXXFLAGS) $(INCLUDES) \
		-isystem $(GTEST_ROOT)/include $^ -o $@
unittest: $(unittest_objects) $(cache_simulation_a) $(kernels_a) $(matrix_a) $(util_a) gtest_main.a
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDFLAGS) -o $@


# Clean
.PHONY: clean
clean:
	rm -f $(util_c_objects) $(util_cxx_objects) $(util_a)
	rm -f $(matrix_objects) $(matrix_a)
	rm -f $(cache_simulation_objects) $(cache_simulation_a)
	rm -f $(kernels_objects) $(kernels_a)
	rm -f $(spmv_cache_trace_objects)
	rm -f spmv-cache-trace
	rm -f gtest_main.o gtest_main.a gtest-all.o
	rm -f $(unittest_objects)
	rm -f unittest
