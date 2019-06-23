# Configuration
CXXFLAGS=-O2 -g -fopenmp -Wall -fsanitize=address -DUSE_POSIX_MEMALIGN
INCLUDES=-Isrc
LDFLAGS=-lz

# Default
.PHONY: all
all: spmv-cache-trace


# Libraries
util_a = src/util/util.a
util_sources = \
	src/util/tarstream.cpp \
	src/util/zlibstream.cpp
util_headers = \
	src/util/tarstream.hpp \
	src/util/zlibstream.hpp
util_objects := \
	$(foreach source,$(util_sources),$(source:.cpp=.o))

$(util_objects): %.o: %.cpp $(util_headers)
	$(CXX) -c $(CXXFLAGS) $(INCLUDES) $< -o $@
$(util_a): $(util_objects)
	$(AR) $(ARFLAGS) $@ $^

matrix_a = src/matrix/matrix.a
matrix_sources = \
	src/matrix/coordinate-matrix.cpp \
	src/matrix/csr-matrix.cpp \
	src/matrix/csr-matrix-spmv.cpp \
	src/matrix/ellpack-matrix.cpp \
	src/matrix/source-vector-only-matrix.cpp \
	src/matrix/matrix.cpp \
	src/matrix/matrix-error.cpp \
	src/matrix/matrix-market.cpp
matrix_headers = \
	src/matrix/coordinate-matrix.hpp \
	src/matrix/csr-matrix.hpp \
	src/matrix/ellpack-matrix.hpp \
	src/matrix/source-vector-only-matrix.hpp \
	src/matrix/matrix.hpp \
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

# Main
spmv_cache_trace_sources = \
	src/spmv-cache-trace.cpp \
	src/main.cpp
spmv_cache_trace_headers = \
	src/spmv-cache-trace.hpp
spmv_cache_trace_objects := \
	$(foreach source,$(spmv_cache_trace_sources),$(source:.cpp=.o))

$(spmv_cache_trace_objects): %.o: %.cpp $(spmv_cache_trace_headers)
	$(CXX) -c $(CXXFLAGS) $(INCLUDES) $< -o $@
spmv-cache-trace: $(spmv_cache_trace_objects) $(cache_simulation_a) $(matrix_a) $(util_a)
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
	test/test_matrix-market.cpp \
	test/test_coordinate-matrix.cpp \
	test/test_csr-matrix.cpp \
	test/test_ellpack-matrix.cpp \
	test/test_replacement.cpp
unittest_objects := \
	$(foreach source,$(unittest_sources),$(source:.cpp=.o))

$(unittest_objects): %.o: %.cpp
	$(CXX) $(CPPFLAGS) -c $(CXXFLAGS) $(INCLUDES) \
		-isystem $(GTEST_ROOT)/include $^ -o $@
unittest: $(unittest_objects) $(cache_simulation_a) $(matrix_a) $(util_a) gtest_main.a 
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $^ $(LDFLAGS) -o $@


# Clean
.PHONY: clean
clean:
	rm -f $(util_objects) $(util_a)
	rm -f $(matrix_objects) $(matrix_a)
	rm -f $(cache_simulation_objects) $(cache_simulation_a)
	rm -f $(spmv_cache_trace_objects)
	rm -f spmv-cache-trace
	rm -f gtest_main.o gtest_main.a gtest-all.o
	rm -f $(unittest_objects)
	rm -f unittest
