#ifndef KERNEL_HPP
#define KERNEL_HPP

#include "trace-config.hpp"
#include "cache-simulation/replacement.hpp"

#include <iosfwd>
#include <stdexcept>
#include <string>

class kernel_error
    : public std::runtime_error
{
public:
    kernel_error(std::string const & message) throw();
};

class Kernel
{
public:
    virtual ~Kernel() {}

    virtual void init(
        TraceConfig const & trace_config,
        std::ostream & o,
        bool verbose) = 0;

    virtual void prepare(TraceConfig const & trace_config) = 0;

    virtual void run(TraceConfig const & trace_config) = 0;

    virtual replacement::MemoryReferenceString memory_reference_string(
        TraceConfig const & trace_config,
        int thread,
        int num_threads) const = 0;

    virtual std::string name() const = 0;

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
