#ifndef TRIAD_HPP
#define TRIAD_HPP

#include "kernel.hpp"
#include "trace-config.hpp"
#include "cache-simulation/replacement.hpp"
#include "util/aligned-allocator.hpp"

#include <iosfwd>
#include <string>

namespace triad
{

typedef int32_t size_type;
typedef double value_type;
typedef std::vector<value_type, aligned_allocator<value_type, 64>> value_array_type;

}

class triad_kernel : public Kernel
{
public:
    triad_kernel(triad::size_type num_entries);
    ~triad_kernel();

    void init(std::ostream & o,
              bool verbose) override;
    void prepare() override;
    void run() override;

    replacement::MemoryReferenceString memory_reference_string(
        TraceConfig const & trace_config,
        int thread,
        int num_threads) const override;

    std::string name() const override;
    std::ostream & print(
        std::ostream & o) const override;

private:
    triad::size_type num_entries;
    triad::value_array_type a;
    triad::value_array_type b;
    triad::value_array_type c;
};

#endif
