#include "cache-trace.hpp"
#include "profile-kernel.hpp"
#include "trace-config.hpp"
#include "kernels.hpp"
#include "util/json-ostreambuf.hpp"
#include "util/perf-events.hpp"

#include <argp.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>

char const * argp_program_version = "spmv-cache-trace 1.0";
char const * argp_program_bug_address = "<james@simula.no>";

typedef unsigned int size_type;

struct arguments
{
    arguments()
        : trace_config()
        , kernel()
        , profile(0)
        , warmup(false)
        , flush_caches(false)
        , list_perf_events(false)
        , verbose(false)
    {
    }

    std::string trace_config;
    std::unique_ptr<Kernel> kernel;
    int profile;
    bool warmup;
    bool flush_caches;
    bool list_perf_events;
    bool verbose;
};

enum class short_options
{
    trace_config = 'c',
    profile = 'p',
    verbose = 'v',

    /* Sparse matrix-vector multplication kernels. */
    non_printable_characters = 128,
    list_perf_events,
    warmup,
    flush_caches,
    triad,
    coo,
    coo_atomic,
    csr,
    ellpack,
    mkl_csr,
    hybrid,
};

error_t parse_option(int key, char * arg, argp_state * state)
{
    arguments & args = *(static_cast<arguments *>(state->input));

    switch (key) {
    case int(short_options::trace_config):
        args.trace_config = arg;
        break;

    case int(short_options::profile):
        try {
            args.profile = std::stoul(arg);
        } catch (std::out_of_range const & e) {
            argp_error(state, "profile: %s", strerror(errno));
        } catch (std::invalid_argument const & e) {
            argp_error(state, "Expected 'profile' to be an integer");
        }
        break;

    case int(short_options::warmup):
        args.warmup = true;
        break;

    case int(short_options::flush_caches):
        args.flush_caches = true;
        break;

    case int(short_options::list_perf_events):
        args.list_perf_events = true;
        break;

    case int(short_options::verbose):
        args.verbose = true;
        break;

        /* STREAM-like kernels. */
    case int(short_options::triad):
        {
            triad::size_type num_entries;
            try {
                num_entries = std::stoul(arg);
            } catch (std::out_of_range const & e) {
                argp_error(state, "triad: %s", strerror(errno));
            } catch (std::invalid_argument const & e) {
                argp_error(state, "triad: expected integer");
            }
            args.kernel = std::make_unique<triad_kernel>(
                num_entries);
            break;
        }

        /* Sparse matrix-vector multplication kernels. */
    case int(short_options::coo):
        {
            std::string matrix_path = arg;
            args.kernel = std::make_unique<coo_spmv_kernel>(
                matrix_path);
            break;
        }
    case int(short_options::coo_atomic):
        {
            std::string matrix_path = arg;
            args.kernel = std::make_unique<coo_spmv_atomic_kernel>(
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
    case int(short_options::ellpack):
        {
            std::string matrix_path = arg;
            args.kernel = std::make_unique<ellpack_spmv_kernel>(
                matrix_path);
            break;
        }
    case int(short_options::mkl_csr):
        {
            std::string matrix_path = arg;
            args.kernel = std::make_unique<mkl_csr_spmv_kernel>(
                matrix_path);
            break;
        }
    case int(short_options::hybrid):
        {
            std::string matrix_path = arg;
            args.kernel = std::make_unique<hybrid_spmv_kernel>(
                matrix_path);
            break;
        }

    case ARGP_KEY_END:
        if (args.list_perf_events)
            break;
        if (!args.kernel)
            argp_error(state, "Please specify a kernel");
        if (args.trace_config.empty())
            argp_error(state, "Please specify --trace-config");
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
        {"profile", int(short_options::profile), "N", 0,
         "Measure cache misses using hardware performance counters", 0},
        {"warmup", int(short_options::warmup), nullptr, 0,
         "Warm up the cache before tracing or profiling", 0},
        {"flush-caches", int(short_options::flush_caches),  nullptr, 0,
         "Flush caches between each profiling run", 0},
        {"list-perf-events", int(short_options::list_perf_events), nullptr, 0,
         "Show available hardware performance monitoring events", 0},
        {"verbose", int(short_options::verbose), nullptr, 0,
         "Produce verbose output"},

        {0, 0, 0, 0, "STREAM-like kernels:" },
        {"triad", int(short_options::triad), "N", 0,
         "Triad: a(i)=b(i)+q*c(i), 24 bytes and 2 flops per iteration", 0},

        {0, 0, 0, 0, "Sparse matrix-vector multplication kernels:" },
        {"coo", int(short_options::coo), "PATH", 0, "Coordinate format", 0},
        {"coo-atomic", int(short_options::coo_atomic), "PATH", 0, "Coordinate format with atomic writes", 0},
        {"csr", int(short_options::csr), "PATH", 0, "Compressed sparse row", 0},
        {"ellpack", int(short_options::ellpack), "PATH", 0, "ELLPACK", 0},
        {"mkl-csr", int(short_options::mkl_csr), "PATH", 0, "Compressed sparse row using Intel MKL", 0},
        {"hybrid", int(short_options::hybrid), "PATH", 0, "Hybrid of ELLPACK and COO", 0},
        {nullptr}};

    auto arginfo = argp{
        options,
        parse_option,
        nullptr,
        "Estimate CPU cache misses for sparse matrix-vector multiplication"};

    arguments args;
    auto err = argp_parse(&arginfo, argc, argv, 0, nullptr, &args);
    if (err) {
        std::cerr << strerror(err) << '\n';
        return err;
    }

    if (args.list_perf_events) {
        perf::libpfm_context libpfm_context;
        libpfm_context.print_perf_events(std::cout);
        return EXIT_SUCCESS;
    }

    try {
        TraceConfig trace_config = read_trace_config(args.trace_config);

        args.kernel->init(trace_config, std::cout, args.verbose);

        if (args.profile == 0) {
            CacheTrace cache_trace = trace_cache_misses(
                trace_config, *(args.kernel.get()), args.warmup, args.verbose);
            auto o = json_ostreambuf(std::cout);
            std::cout << cache_trace << '\n';
        }
        else {
            perf::libpfm_context libpfm_context;
            Profiling profiling = profile_kernel(
                trace_config,
                *(args.kernel.get()),
                true,
                args.flush_caches,
                args.profile,
                libpfm_context,
                std::cout,
                args.verbose);
            auto o = json_ostreambuf(std::cout);
            std::cout << profiling << '\n';
        }

    } catch (trace_config_error const & e) {
        std::cerr << args.trace_config << ": " << e.what() << '\n';
        return EXIT_FAILURE;
    } catch (kernel_error const & e) {
        std::cerr << args.kernel->name() << ": " << e.what() << '\n';
        return EXIT_FAILURE;
    } catch (perf::perf_error const & e) {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
