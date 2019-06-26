#ifndef KERNEL_HPP
#define KERNEL_HPP

#include "trace-config.hpp"
#include "cache-simulation/replacement.hpp"

class Kernel
{
public:
    virtual ~Kernel() {}

    virtual replacement::MemoryReferenceString memory_reference_string(
        TraceConfig const & trace_config,
        int thread,
        int num_threads,
        int cache_line_size) const = 0;

    virtual std::ostream & print(
        std::ostream & o) const = 0;

    friend std::ostream & operator<<(
        std::ostream & o,
        Kernel const & kernel);
};

std::ostream & operator<<(
    std::ostream & o,
    Kernel const & kernel);

#endif