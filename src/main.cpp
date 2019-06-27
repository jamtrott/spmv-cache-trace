#include "cache-trace.hpp"
#include "trace-config.hpp"
#include "kernels.hpp"
#include "util/json-ostreambuf.hpp"

#include <argp.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace std::literals::string_literals;

char const * argp_program_version = "spmv-cache-trace 1.0";
char const * argp_program_bug_address = "<james@simula.no>";

typedef unsigned int size_type;

struct arguments
{
    arguments()
        : trace_config()
        , kernel()
        , verbose(false)
    {
    }

    std::string trace_config;
    std::unique_ptr<Kernel> kernel;
    bool verbose;
};

enum class short_options
{
    trace_config = 'c',
    verbose = 'v',

    /* Sparse matrix-vector multplication kernels. */
    non_printable_characters = 128,
    coo,
    csr,
};

error_t parse_option(int key, char * arg, argp_state * state)
{
    arguments & args = *(static_cast<arguments *>(state->input));

    switch (key) {
    case int(short_options::trace_config):
        args.trace_config = arg;
        break;

    case int(short_options::verbose):
        args.verbose = true;
        break;

        /* Sparse matrix-vector multplication kernels. */
    case int(short_options::coo):
        {
            std::string matrix_path = arg;
            args.kernel = std::make_unique<coo_spmv_kernel>(
                matrix_path);
            break;
        }
    case int(short_options::csr):
        {
            std::string matrix_path = arg;
            args.kernel = std::make_unique<csr_spmv_kernel>(
                matrix_path);
            break;
        }

    case ARGP_KEY_END:
        if (args.trace_config.empty())
            argp_error(state, "Please specify --trace-config");
        if (!args.kernel)
            argp_error(state, "Please specify a kernel");
        break;

    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

int main(int argc, char ** argv)
{
    argp_option options[] = {
        {"trace-config", int(short_options::trace_config), "PATH", 0,
         "Read cache parameters from a configuration file in JSON format."},
        {"verbose", int(short_options::verbose), nullptr, 0,
         "Produce verbose output"},

        {0, 0, 0, 0, "Sparse matrix-vector multplication kernels:" },
        {"coo", int(short_options::coo), "PATH", 0, "Coordinate format", 0},
        {"csr", int(short_options::csr), "PATH", 0, "Compressed sparse row", 0},
        {nullptr}};

    auto arginfo = argp{
        options,
        parse_option,
        nullptr,
        "spmv-cache-trace -- "
        "Estimate CPU cache misses for sparse matrix-vector multiplication"};

    arguments args;
    auto err = argp_parse(&arginfo, argc, argv, 0, nullptr, &args);
    if (err) {
        std::cerr << strerror(err) << '\n';
        return err;
    }

    try {
        TraceConfig trace_config = read_trace_config(args.trace_config);
        args.kernel->init(std::cout, args.verbose);
        CacheTrace cache_trace = trace_cache_misses(
            trace_config, *(args.kernel.get()));
        auto o = json_ostreambuf(std::cout);
        std::cout << cache_trace << '\n';
    } catch (trace_config_error const & e) {
        std::cerr << args.trace_config << ": " << e.what() << '\n';
        return EXIT_FAILURE;
    } catch (kernel_error const & e) {
        std::cerr << args.kernel->name() << ": " << e.what() << '\n';
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
