#include "cache-trace.hpp"
#include "trace-config.hpp"
#include "spmv-kernel.hpp"

#include "matrix/matrix.hpp"
#include "matrix/matrix-error.hpp"
#include "matrix/matrix-market.hpp"
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
        , matrix_format(matrix::MatrixFormat::csr)
        , list_matrix_formats(false)
        , matrix_path()
        , verbose(false)
    {
    }

    std::string trace_config;
    matrix::MatrixFormat matrix_format;
    bool list_matrix_formats;
    std::string matrix_path;
    bool verbose;
};

enum class short_options
{
    trace_config = 'c',
    matrix_format = 'f',
    list_matrix_formats = 'l',
    matrix = 'm',
    verbose = 'v',
};

error_t parse_option(int key, char * arg, argp_state * state)
{
    arguments & args = *(static_cast<arguments *>(state->input));

    switch (key) {
    case int(short_options::trace_config):
        args.trace_config = arg;
        break;

    case int(short_options::matrix_format):
        {
            auto format = std::string(arg);
            std::transform(
                std::begin(format), std::end(format),
                std::begin(format),
                [] (unsigned char c) { return std::tolower(c); });
            args.matrix_format = matrix::find_matrix_format(format);
            if (args.matrix_format == matrix::MatrixFormat::none)
                argp_error(state, "Unknown sparse matrix format: %s", arg);
        }
        break;

    case int(short_options::list_matrix_formats):
        args.list_matrix_formats = true;
        break;

    case int(short_options::matrix):
        args.matrix_path = arg;
        break;

    case int(short_options::verbose):
        args.verbose = true;
        break;

    case ARGP_KEY_END:
        if (args.list_matrix_formats)
            break;
        if (!args.matrix_path.empty())
            break;
        if (!args.trace_config.empty())
            break;
        argp_usage(state);
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
        {"matrix", int(short_options::matrix), "PATH", 0,
         "Load a matrix from a file in matrix market format "
         "or from a compressed tarball containing such a file.", 0},
        {"matrix-format", int(short_options::matrix_format),
         "FORMAT", 0, "Sparse matrix format (default: CSR)", 0},
        {"list-matrix-formats", int(short_options::list_matrix_formats),
         nullptr, 0, "List available sparse matrix formats", 0},
        {"verbose", int(short_options::verbose), nullptr, 0,
         "Produce verbose output"},
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

    if (args.list_matrix_formats) {
        matrix::list_matrix_formats(std::cout);
        return EXIT_SUCCESS;
    }

    try {
        TraceConfig trace_config = read_trace_config(args.trace_config);

        std::unique_ptr<Kernel> kernel;
        try {
            kernel = std::make_unique<SpMV>(
                args.matrix_path,
                args.matrix_format,
                std::cout,
                args.verbose);
        } catch (matrix_market::matrix_market_error & e) {
            std::cerr << args.matrix_path << ": " << e.what() << '\n';
            return EXIT_FAILURE;
        } catch (matrix::matrix_error & e) {
            std::cerr << args.matrix_path << ": "
                      << args.matrix_format << ": "
                      << e.what() << '\n';
            return EXIT_FAILURE;
        } catch (std::system_error & e) {
            std::cerr << args.matrix_path << ": "
                      << args.matrix_format << ": "
                      << e.what() << '\n';
            return EXIT_FAILURE;
        }

        CacheTrace cache_trace = trace_cache_misses(
            trace_config, *(kernel.get()));
        auto o = json_ostreambuf(std::cout);
        std::cout << cache_trace << '\n';

    } catch (trace_config_error & e) {
        std::cerr << args.trace_config << ": " << e.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
