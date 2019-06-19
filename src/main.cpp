#include "spmv-cache-trace.hpp"

#include "matrix/matrix.hpp"
#include "matrix/matrix-market.hpp"

#include <argp.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <numeric>
#include <stdexcept>
#include <string>
#include <sstream>
#include <vector>

using namespace std::literals::string_literals;

char const * argp_program_version = "spmv-cache-trace 1.0";
char const * argp_program_bug_address = "<james@simula.no>";

struct arguments
{
    arguments()
        : matrix_format(matrix::MatrixFormat::csr)
        , list_matrix_formats(false)
        , matrix_path()
        , cache_size(32768)
        , cache_line_size(64)
        , shared_cache(false)
        , threads(1u)
        , verbose(false)
    {
    }

    matrix::MatrixFormat matrix_format;
    bool list_matrix_formats;
    std::string matrix_path;
    unsigned int cache_size;
    unsigned int cache_line_size;
    bool shared_cache;
    unsigned int threads;
    bool verbose;
};

enum class short_options
{
    cache_size = 'Z',
    cache_line_size = 'L',
    matrix_format = 'f',
    list_matrix_formats = 'l',
    matrix = 'm',
    threads = 't',
    shared_cache = 's',
    verbose = 'v',
};

error_t parse_option(int key, char * arg, argp_state * state)
{
    arguments & args = *(static_cast<arguments *>(state->input));

    switch (key) {
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

    case int(short_options::cache_size):
        args.cache_size = std::stoull(arg);
        break;

    case int(short_options::cache_line_size):
        args.cache_line_size = std::stoull(arg);
        break;

    case int(short_options::threads):
        args.threads = std::max(std::stoull(arg), 1ull);
        break;

    case int(short_options::shared_cache):
        args.shared_cache = true;
        break;

    case int(short_options::verbose):
        args.verbose = true;
        break;

    case ARGP_KEY_END:
        if (args.list_matrix_formats)
            break;
        if (!args.matrix_path.empty())
            break;
        argp_usage(state);
        break;

    default:
        return ARGP_ERR_UNKNOWN;
    }
    return 0;
}

auto load_spmv_data(
    std::string const & matrix_path,
    matrix::MatrixFormat const & matrix_format,
    std::ostream & o,
    bool verbose)
{
    auto market_matrix = matrix_market::load_matrix(
        matrix_path, o, verbose);
    auto A = from_matrix_market(
        market_matrix, matrix_format, o, verbose);
    auto x = matrix::make_vector(
        matrix_format, std::vector<double>(market_matrix.columns(), 1.0));
    auto y = matrix::make_vector(
        matrix_format, std::vector<double>(market_matrix.rows(), 0.0));
    return std::make_tuple(std::move(A), x, y);
}

int main(int argc, char ** argv)
{
    argp_option options[] = {
        {"cache-size", int(short_options::cache_size),
         "Z", 0, "Estimate the number of cache misses for a cache of size Z."},
        {"cache-line-size", int(short_options::cache_line_size),
         "L", 0, "Estimate the number of cache misses with cache lines of size L."},
        {"matrix", int(short_options::matrix),
         "PATH", 0, "Load a matrix from a file in matrix market format "
         "or from a compressed tarball containing such a file.", 0},
        {"matrix-format", int(short_options::matrix_format),
         "FORMAT", 0, "Sparse matrix format (default: CSR)", 0},
        {"list-matrix-formats", int(short_options::list_matrix_formats),
         nullptr, 0, "List available sparse matrix formats", 0},
        {"threads", int(short_options::threads), "N", 0,
         "Produce estimates for a parallel SpMV with N threads"},
        {"shared-cache", int(short_options::shared_cache), nullptr, 0,
         "Assume that the cache is shared between threads"},
        {"verbose", int(short_options::verbose), nullptr, 0, "Produce verbose output"},
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
        return 0;
    }

    auto matrix_path = args.matrix_path;
    auto matrix_format = args.matrix_format;
    auto cache_size = args.cache_size;
    auto cache_line_size = args.cache_line_size;
    auto shared_cache = args.shared_cache;
    auto num_threads = args.threads;
    auto verbose = args.verbose;

    try {
        auto p = load_spmv_data(
            matrix_path, matrix_format, std::cout, args.verbose);
        auto A = std::move(std::get<0>(p));
        auto x = std::move(std::get<1>(p));
        auto y = std::move(std::get<2>(p));

        auto matrix_format_s = matrix::matrix_format_name(matrix_format);
        auto rows = A.rows();
        auto columns = A.columns();
        auto nonzeros = A.nonzeros();
        auto matrix_size = A.size();
        auto cache_misses = estimate_spmv_cache_complexity(
            A, x, y, num_threads,
            cache_size, cache_line_size, shared_cache);

        std::cout
            << "{\n"
            << "  \"matrix_path\": \"" << matrix_path << "\",\n"
            << "  \"matrix_format\": \"" << matrix_format_s << "\",\n"
            << "  \"rows\": " << rows << ",\n"
            << "  \"columns\": " << columns << ",\n"
            << "  \"nonzeros\": " << nonzeros << ",\n"
            << "  \"matrix_size\": " << matrix_size << ",\n"
            << "  \"threads\": " << num_threads << ",\n"
            << "  \"cache_size\": " << cache_size << ",\n"
            << "  \"cache_line_size\": " << cache_line_size << ",\n"
            << "  \"shared_cache\": " << (shared_cache ? "true" : "false") << ",\n"
            << "  \"cache_misses\": " << cache_misses
            << "\n}";

    } catch (std::exception & e) {
        std::cerr << e.what() << std::endl;
        return -1;
    }
}
