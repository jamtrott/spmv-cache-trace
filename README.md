spmv-cache-trace
================
The code in this repository is used to perform trace-based simulations of cache misses for parallel computations with irregular memory access patterns. Primarily, we consider a few different **Sparse Matrix-Vector Multiplication (SpMV)** kernels for different sparse matrix storage formats. These formats include *coordinate (COO)*, *compressed sparse row (CSR)*, and *ELLPACK*.

Based on a few, reasonable assumptions, we estimate the number of cache misses incurred by each thread in a parallel computation with irregular memory access patterns. This is achieved by simulating the behaviour of a memory hierarchy which may consist of both private and shared caches.

Table of contents
-----------------
   * [Installation](#installation)
   * [Usage](#usage)
      * [Sparse matrix-vector multiplication](#sparse-matrix-vector-multiplication)
      * [Trace configuration](#trace-configuration)
      * [Cache tracing](#cache-tracing)
      * [Profiling](#profiling)


Installation
============
To build, simply run `make`. To run the unit tests, run `make check`.

For the unit tests, it may be necessary to set the environment variable `GTEST_ROOT` to point to the Google Test directory.

#### Dependencies
  * zlib (https://zlib.net/)
  * libpfm4 (http://perfmon2.sourceforge.net/)
  * Google Test (https://github.com/google/googletest) for unit tests.

Tested on Ubuntu 18.04.2 LTS (GNU/Linux 4.18.0-20-generic x86_64).


### Intel C compiler and Math Kernel Library
To build with the Intel C compiler, run `CC=icc CXX=icpc make`. Additionally, to use the Intel Math Kernel Library (MKL), set `MKLROOT` to point to the MKL directory, and set `USE_INTEL_MKL=1`.

Usage
=====

Sparse matrix-vector multiplication
-----------------------------------
The sparse matrix-vector multiplication kernels require a matrix stored in the *matrix market* format. Alternatively, a gzip-compressed tarball may be provided that contains a directory and a matrix within that directory that have the same name as the tarball. For example, `test.tar.gz` should contain the matrix `test/test.mtx`. This is the format provided by the *SuiteSparse Matrix Collection* (https://sparse.tamu.edu/).

Trace configuration
-------------------
The `spmv-cache-trace` program uses a *trace configuration* to describe the memory hierarchy and the set of threads to use for a given simulation. The configuration is given by a file in JSON format, as shown below.

The trace configuration consists of three parts:

1. `"numa_domains"` describe one or more regions that partition the main memory in case of a *non-uniform memory access (NUMA)* system. These are typically used to distinguish between local and remote memory accesses in a multi-socket system.

1. `"caches"` describe the size of each cache and its cache lines, as well as how caches are shared. The cache sizes and cache line sizes are given in bytes. For each cache, `"parents"` is either the name of a cache or the names of one or more NUMA domains, which, in either case, represents the cache or NUMA domains to search whenever a cache miss occurs.

1. `"thread_affinities"` describe the number of threads, as well as specifying which (first-level) cache and NUMA domain that each thread belongs to.

The following example is a trace configuration for a simulation that might correspond to using two threads on a two-socket system with a shared third-level cache:
```json
{
  "numa_domains": ["DRAM-0", "DRAM-1"],
  "caches": {
    "L1-0": {"size":    32768, "line_size": 64, "parents": ["L2-0"]},
    "L1-1": {"size":    32768, "line_size": 64, "parents": ["L2-1"]},
    "L2-0": {"size":   262144, "line_size": 64, "parents": ["L3"]},
    "L2-1": {"size":   262144, "line_size": 64, "parents": ["L3"]},
    "L3":   {"size": 20971520, "line_size": 64, "parents": ["DRAM-0", "DRAM-1"]}
  },
  "thread_affinities": [
    {"thread": 0, "cpu": 0, "cache": "L1-0", "numa_domain": "DRAM-0"},
    {"thread": 1, "cpu": 1, "cache": "L1-1", "numa_domain": "DRAM-1"}
  ]
}
```

Cache tracing
-------------
The command
```bash
./spmv-cache-trace --trace-config trace-config.json --csr 1138_bus.tar.gz
```
will perform a cache trace for a sparse matrix-vector multiplication in the compressed sparse row format using the matrix `1138_bus.tar.gz`. If the trace configuration provided by `trace-config.json` is the same as the one shown above, then the following output is produced:
```json
{
  "trace-config": {
    "numa_domains": ["DRAM-0", "DRAM-1"],
    "caches": {
      "L1-0": {"size": 32768, "line_size": 64, "parents": ["L2-0"], "events": []},
      "L1-1": {"size": 32768, "line_size": 64, "parents": ["L2-1"], "events": []},
      "L2-0": {"size": 262144, "line_size": 64, "parents": ["L3"], "events": []},
      "L2-1": {"size": 262144, "line_size": 64, "parents": ["L3"], "events": []},
      "L3": {"size": 20971520, "line_size": 64, "parents": ["DRAM-0", "DRAM-1"], "events": []}
    },
    "thread_affinities": [
      {"cpu": 0, "cache": "L1-0", "numa_domain": "DRAM-0", "event_groups": []},
      {"cpu": 1, "cache": "L1-1", "numa_domain": "DRAM-1", "event_groups": []}
    ]
  },
  "kernel": {
    "name": "spmv",
    "matrix_path": "1138_bus.tar.gz",
    "matrix_format": "csr",
    "rows": 1138,
    "columns": 1138,
    "nonzeros": 2596,
    "matrix_size": 35708
  },
  "cache_misses": {
    "L1-0": [[423, 0], [0, 0]],
    "L1-1": [[0, 0], [35, 427]],
    "L2-0": [[423, 0], [0, 0]],
    "L2-1": [[0, 0], [35, 427]],
    "L3": [[396, 0], [23, 427]]
  }
}
```
For each cache, the cache misses are given for each combination of thread and NUMA domain. Thus, for the third-level cache, the first thread incurred 396 cache misses that would have to be fetched from the first NUMA domain, and none for the second NUMA domain. The second thread incurred 23 cache misses for the first NUMA domain, and 427 for the second NUMA domain.

Profiling
-------------
The command
```bash
./spmv-cache-trace --trace-config trace-config.json --csr 1138_bus.tar.gz --profile=10
```
will profile ten runs of the sparse matrix-vector multiplication kernel for the compressed sparse row format using the matrix `1138_bus.tar.gz`. If the trace configuration provided by `trace-config.json` is the same as the one shown above, then the following output is produced:
```json
{
  "trace_config": {
    "numa_domains": ["DRAM-0", "DRAM-1"],
    "caches": {
      "L1-0": {"size": 32768, "line_size": 64, "parents": ["L2-0"], "events": []},
      "L1-1": {"size": 32768, "line_size": 64, "parents": ["L2-1"], "events": []},
      "L2-0": {"size": 262144, "line_size": 64, "parents": ["L3"], "events": []},
      "L2-1": {"size": 262144, "line_size": 64, "parents": ["L3"], "events": []},
      "L3": {"size": 20971520, "line_size": 64, "parents": ["DRAM-0", "DRAM-1"], "events": []}
    },
    "thread_affinities": [
      {"cpu": 0, "cache": "L1-0", "numa_domain": "DRAM-0", "event_groups": []},
      {"cpu": 1, "cache": "L1-1", "numa_domain": "DRAM-1", "event_groups": []}
    ]
  },
  "kernel": {
    "name": "spmv",
    "matrix_path": "1138_bus.tar.gz",
    "matrix_format": "csr",
    "rows": 1138,
    "columns": 1138,
    "nonzeros": 2596,
    "matrix_size": 35708
  },
  "execution_time": {
    "samples": 10,
    "min": 14155,
    "max": 21658,
    "mean": 15283.7,
    "median": 14252,
    "variance": 5.56577e+06,
    "standard_deviation": 2359.19,
    "skewness": 1.91989,
    "kurtosis": 6.62483,
    "unit": "ns"
  },
  "profiling_events": [],
  "cache_misses": {
    "L1-0": [],
    "L1-1": [],
    "L2-0": [],
    "L2-1": [],
    "L3": []
  }
}
```
The data shown under `"execution_time"` are statistics based on the execution times of the kernel runs. For instance, the mean execution time is 15 284 ns, or about 15 microseconds.

### Hardware performance monitoring
In addition, it is possible to obtain more profiling information by using hardware performance monitoring events. To do so, each thread in the trace configuration may provide an array called `"event_groups"`. Each event group is an array of strings, where each string names a hardware performance event according to the conventions used by the library *libpfm4*.

The following example shows a single thread configured with two event groups:
```json
{
  "numa_domains": ["DRAM"],
  "caches": {
    "L1": {"size":    32768, "line_size": 64, "parents": ["L2"]},
    "L2": {"size":   262144, "line_size": 64, "parents": ["L3"]},
    "L3":   {"size": 20971520, "line_size": 64, "parents": ["DRAM"]}
  },
  "thread_affinities": [
    {"thread": 0, "cpu": 0, "cache": "L1-0", "numa_domain": "DRAM-0",
     "event_groups": [["l1-dcache-loads", "l1-dcache-load-misses"], ["llc-load-misses"]]}
  ]
}
```
In the above example, the first event group will measure the events `"l1-dcache-loads"` and `"l1-dcache-load-misses"`, which, roughly speaking, correspond to the number of loads that access the L1 cache and the number of loads that miss the L1 cache, respectively. The second event group measures the event `"llc-load-misses"`, which should correspond to the number of loads that miss the last-level cache.

To see a list of available events, use `./spmv-cache-trace --list-perf-events`.
