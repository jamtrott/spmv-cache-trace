# spmv-cache-trace
The code in this repository is used to perform trace-based simulations of cache misses for parallel computations with irregular memory access patterns. Primarily, we consider a few different **Sparse Matrix-Vector Multiplication (SpMV)** kernels for different sparse matrix storage formats. These formats include *coordinate (COO)*, *compressed sparse row (CSR)*, and *ELLPACK*.

Based on a few, reasonable assumptions, we estimate the number of cache misses incurred by each thread in a parallel computation with irregular memory access patterns. This is achieved by simulating the behaviour of a memory hierarchy which may consist of both private and shared caches.

## Trace configuration
The `spmv-cache-trace` program uses a *trace configuration* to define the memory hierarchy and the set of threads to use for a given simulation. The configuration is given by a file in JSON format. For example, the following shows a trace configuration that can be used for a simulation with two threads using a cache hierarchy with three levels of caches:
```json
"trace-config": {
  "caches": {
    "L1-0": {"size":    32768, "line_size": 64, "parents": ["L2-0"]},
    "L1-1": {"size":    32768, "line_size": 64, "parents": ["L2-1"]},
    "L2-0": {"size":   262144, "line_size": 64, "parents": ["L3"]},
    "L2-1": {"size":   262144, "line_size": 64, "parents": ["L3"]},
    "L3":   {"size": 20971520, "line_size": 64, "parents": []}
  },
  "thread_affinities": ["L1-0", "L1-1"]
}
```
First, each cache is specified by its size (in bytes), the size of its cache lines (in bytes), and its "parents", which is the next set of caches that are searched whenever a cache miss occurs. Second, each thread that takes part in the parallel computation is given a "thread affinity", specifying which (first-level) cache that the thread belongs to.

## Sparse matrix-vector multiplication
The sparse matrix-vector multiplication kernels require a matrix stored in the *matrix market* format. Alternatively, a gzip-compressed tarball may be provided that contains a directory and a matrix within that directory that have the same name as the tarball. For example, `test.tar.gz` should contain the matrix `test/test.mtx`. This is the format provided by the *SuiteSparse Matrix Collection* (https://sparse.tamu.edu/).

## Example
Assuming that `trace-config.json` contains the trace configuration given above, and a matrix is given by `suitesparse/HB/1138_bus.tar.gz` (see https://sparse.tamu.edu/HB/1138_bus), then the command
```sh
$ ./spmv-cache-trace --trace-config trace-config.json -m suitesparse/HB/1138_bus.tar.gz
```
will produce the following output:
```json
"trace-config": {
  "caches": {
    "L1-0": {"size": 32768, "line_size": 64, "parents": ["L2-0"]},
    "L1-1": {"size": 32768, "line_size": 64, "parents": ["L2-1"]},
    "L2-0": {"size": 262144, "line_size": 64, "parents": ["L3"]},
    "L2-1": {"size": 262144, "line_size": 64, "parents": ["L3"]},
    "L3": {"size": 20971520, "line_size": 64, "parents": []}
  },
  "thread_affinities": ["L1-0", "L1-1"]
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
  "L1-0": [423, 0],
  "L1-1": [0, 462],
  "L2-0": [423, 0],
  "L2-1": [0, 462],
  "L3": [396, 450]
}
```
From the `"cache-misses"`, we may deduce that the first thread incurred 423 cache misses for the first and second-level caches, and 396 cache misses for the third-level cache. The corresponding numbers for the second thread are 462 cache misses for the first- and second-level caches, and 450 for the third-level cache.
