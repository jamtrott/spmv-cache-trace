# spmv-cache-trace
The code in this repository is used to perform trace-based simulations of cache misses for parallel computations with irregular memory access patterns. Primarily, we consider a few different **Sparse Matrix-Vector Multiplication (SpMV)** kernels for different sparse matrix storage formats. These formats include *coordinate (COO)*, *compressed sparse row (CSR)*, and *ELLPACK*.

Based on a few, reasonable assumptions, we estimate the number of cache misses incurred by each thread in a parallel computation with irregular memory access patterns. This is achieved by simulating the behaviour of a memory hierarchy which may consist of both private and shared caches.

## Trace configuration
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

## Sparse matrix-vector multiplication
The sparse matrix-vector multiplication kernels require a matrix stored in the *matrix market* format. Alternatively, a gzip-compressed tarball may be provided that contains a directory and a matrix within that directory that have the same name as the tarball. For example, `test.tar.gz` should contain the matrix `test/test.mtx`. This is the format provided by the *SuiteSparse Matrix Collection* (https://sparse.tamu.edu/).

## Example
Assuming that `trace-config.json` contains the trace configuration given above, and a matrix is given by `suitesparse/HB/1138_bus.tar.gz` (see https://sparse.tamu.edu/HB/1138_bus), then the command
```sh
$ ./spmv-cache-trace --trace-config trace-config.json -m suitesparse/HB/1138_bus.tar.gz
```
will produce the following output:
```json
{
  "trace-config": {
    "caches": {
      "L1-0": {"size": 32768, "line_size": 64, "parents": ["L2-0"]},
      "L1-1": {"size": 32768, "line_size": 64, "parents": ["L2-1"]},
      "L2-0": {"size": 262144, "line_size": 64, "parents": ["L3"]},
      "L2-1": {"size": 262144, "line_size": 64, "parents": ["L3"]},
      "L3": {"size": 20971520, "line_size": 64, "parents": []}
    },
    "numa_domains": ["DRAM-0", "DRAM-1"],
    "thread_affinities": [
      {"cache": "L1-0", "numa_domain": "DRAM-0"},
      {"cache": "L1-1", "numa_domain": "DRAM-1"}
    ]
  },
  "kernel": {
    "name": "spmv",
    "matrix_path": "/work/data/suitesparse/HB/1138_bus.tar.gz",
    "matrix_format": "csr",
    "rows": 1138,
    "columns": 1138,
    "nonzeros": 2596,
    "matrix_size": 35708
  },
  "cache-misses": {
    "L1-0": [[423, 0], [0, 0]],
    "L1-1": [[0, 0], [35, 427]],
    "L2-0": [[423, 0], [0, 0]],
    "L2-1": [[0, 0], [35, 427]],
    "L3": [[396, 0], [23, 427]]
  }
}
```
For each cache, the cache misses are given for each combination of thread and NUMA domain. Thus, for the third-level cache, the first thread incurred 396 cache misses that would have to be fetched from the first NUMA domain, and none for the second NUMA domain. The second thread incurred 23 cache misses for the first NUMA domain, and 427 for the second NUMA domain.
