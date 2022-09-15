#include "cache-trace.hpp"
#include "profile-kernel.hpp"
#include "trace-config.hpp"
#include "kernels.hpp"
#include "util/json-ostreambuf.hpp"
#include "util/perf-events.hpp"

#include <argp.h>

#include <locale.h>

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

char const * argp_program_version = "spmv-cache-trace 2.0";
char const * argp_program_bug_address = "<james@simula.no>";

typedef unsigned int size_type;

enum kernel_type
{
    kernel_triad,
    kernel_coo,
    kernel_coo_atomic,
    kernel_csr,
    kernel_ell,
    kernel_mkl_csr,
    kernel_hybrid,
};

struct arguments
{
    arguments()
        : kernel_type(kernel_triad)
        , N(0)
        , matrix_path()
        , trace_config()
        , profile(0)
        , warmup(false)
        , flush_caches(false)
        , list_perf_events(false)
        , verbose(false)
        , progress_interval(1)
    {
    }

    enum kernel_type kernel_type;
    size_type N;
    std::string matrix_path;
    std::string trace_config;
    int profile;
    bool warmup;
    bool flush_caches;
    bool list_perf_events;
    bool verbose;
    int progress_interval;
};

enum class short_options
{
    matrix = 'm',
    trace_config = 'c',
    profile = 'p',
    verbose = 'v',

    /* Sparse matrix-vector multplication kernels. */
    non_printable_characters = 128,
    list_perf_events,
    warmup,
    flush_caches,
    triad,
    spmv_format,
};

error_t parse_option(int key, char * arg, argp_state * state)
{
    arguments & args = *(static_cast<arguments *>(state->input));

    switch (key) {
    case int(short_options::matrix):
        args.matrix_path = arg;
        break;

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
            args.kernel_type = kernel_triad;
            triad::size_type num_entries = 0;
            try {
                num_entries = std::stoul(arg);
            } catch (std::out_of_range const & e) {
                argp_error(state, "triad: %s", strerror(errno));
            } catch (std::invalid_argument const & e) {
                argp_error(state, "triad: expected integer");
            }
            args.N = num_entries;
            break;
        }

        /* Sparse matrix-vector multplication kernels. */
    case int(short_options::spmv_format):
        if (strcmp(arg, "coo") == 0) args.kernel_type = kernel_coo;
        else if (strcmp(arg, "coo-atomic") == 0) args.kernel_type = kernel_coo_atomic;
        else if (strcmp(arg, "csr") == 0) args.kernel_type = kernel_csr;
        else if (strcmp(arg, "ell") == 0) args.kernel_type = kernel_ell;
        else if (strcmp(arg, "mkl-csr") == 0) args.kernel_type = kernel_mkl_csr;
        else if (strcmp(arg, "hybrid") == 0) args.kernel_type = kernel_hybrid;
        else argp_error(state, "invalid argument");
        break;

    case ARGP_KEY_END:
        if (args.list_perf_events)
            break;
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
    setlocale(LC_ALL, "");

    argp_option options[] = {
        {"matrix", int(short_options::matrix), "PATH", 0,
         "Read matrix from file in Matrix Market format."},
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
         "be more verbose"},

        {0, 0, 0, 0, "STREAM-like kernels:" },
        {"triad", int(short_options::triad), "N", 0,
         "Triad: a(i)=b(i)+q*c(i), 24 bytes and 2 flops per iteration", 0},

        {0, 0, 0, 0, "Sparse matrix-vector multplication kernels:" },
        {"spmv-format", int(short_options::spmv_format), "FMT", 0, "choose one of: coo, coo-atomic, csr, ell, mkl-csr and hybrid", 0},
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

    std::unique_ptr<Kernel> kernel;
    switch (args.kernel_type) {
    case kernel_triad:
        kernel = std::make_unique<triad_kernel>(args.N);
        break;
    case kernel_coo:
        kernel = std::make_unique<coo_spmv_kernel>(args.matrix_path);
        break;
    case kernel_coo_atomic:
        kernel = std::make_unique<coo_spmv_atomic_kernel>(args.matrix_path);
        break;
    case kernel_csr:
        kernel = std::make_unique<csr_spmv_kernel>(args.matrix_path);
        break;
    case kernel_ell:
        kernel = std::make_unique<ell_spmv_kernel>(args.matrix_path);
        break;
    case kernel_mkl_csr:
        kernel = std::make_unique<mkl_csr_spmv_kernel>(args.matrix_path);
        break;
    case kernel_hybrid:
        kernel = std::make_unique<hybrid_spmv_kernel>(args.matrix_path);
        break;
    }

    try {
        TraceConfig trace_config = read_trace_config(args.trace_config);

        kernel->init(trace_config, std::cerr, args.verbose);

        if (args.profile == 0) {
            CacheTrace cache_trace = trace_cache_misses(
                trace_config, *(kernel.get()), args.warmup,
                args.verbose, args.progress_interval);
            auto o = json_ostreambuf(std::cout);
            std::cout << cache_trace << '\n';
        }
        else {
            perf::libpfm_context libpfm_context;
            Profiling profiling = profile_kernel(
                trace_config,
                *(kernel.get()),
                true,
                args.flush_caches,
                args.profile,
                libpfm_context,
                std::cerr,
                args.verbose);
            auto o = json_ostreambuf(std::cout);
            std::cout << profiling << '\n';
        }

    } catch (trace_config_error const & e) {
        std::cerr << args.trace_config << ": " << e.what() << '\n';
        return EXIT_FAILURE;
    } catch (kernel_error const & e) {
        std::cerr << kernel->name() << ": " << e.what() << '\n';
        return EXIT_FAILURE;
    } catch (perf::perf_error const & e) {
        std::cerr << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
