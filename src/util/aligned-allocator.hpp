#ifndef ALIGNED_ALLOCATOR_HPP
#define ALIGNED_ALLOCATOR_HPP

#include <omp.h>
#include <numa.h>
#include <numaif.h>

#include <cstddef>
#include <cstdlib>
#include <limits>
#include <new>
#include <system_error>
#include <errno.h>

#include <iostream>

/*
 * An allocator for memory with specific alignment requirements.  This
 * can be used together with standard library containers, such as
 * std::vector.
 *
 * For example, when using intrinsics for Intel's Advanced Vector
 * Extensions the data must be aligned to 16-, 32-, or 64-byte
 * boundaries.  A container for __m256d, a vector data type that holds
 * four double precision floating-point values, with a 32-byte aligned
 * storage can be declared as follows:
 *
 *     std::vector<__m256d, aligned_allocator<__m256d, 32>> x;
 *
 * The allocator also implements an optional "first touch", which lets
 * each OpenMP thread write to the region of the which it owns.  This
 * is done so that the data belonging to each thread is mapped to the
 * memory controller closest to the CPU core on which the thread is
 * running.
 */
template <typename T, int alignment, bool first_touch = true>
class aligned_allocator
{
public:

    using value_type = T;
    using pointer = T *;
    using const_pointer = T const *;
    using reference = T &;
    using const_reference = T const &;
    using size_type = std::size_t;
    using difference_type = ptrdiff_t;

    template <class U>
    struct rebind
    {
        typedef aligned_allocator<U,alignment,first_touch> other;
    };

    aligned_allocator() = default;
    aligned_allocator(aligned_allocator const &) {}
    template <typename U> aligned_allocator(
        aligned_allocator<U, alignment> const &) {}
    aligned_allocator(aligned_allocator &&) {}
    template <typename U> aligned_allocator(
        aligned_allocator<U, alignment, first_touch> &&) {}

    pointer allocate(size_type n)
    {
        pointer p = nullptr;

#ifdef USE_ALIGNED_ALLOC
        p = reinterpret_cast<pointer>(
            aligned_alloc(alignment, n * sizeof(value_type)));
        if (!p)
            throw std::system_error(errno, std::generic_category());
#elif USE_POSIX_MEMALIGN
        int ret = posix_memalign(
            (void **) &p, alignment, n * sizeof(value_type));
        if (ret != 0)
            throw std::system_error(ret, std::generic_category());
#else
#error "Missing support for aligned memory allocation"
#endif

        if (first_touch)
            touch(p, n);

        return p;
    }

    void deallocate(pointer p, size_type)
    {
        std::free(p);
    }

    void construct(
        pointer p,
        const_reference value)
    {
        new (p) value_type(value);
    }

    void destroy(pointer p)
    {
        p->~value_type();
    }

    size_type max_size() const
    {
        return (std::numeric_limits<size_type>::max()
                / sizeof(value_type));
    }

private:
    void touch(pointer p, size_type n)
    {
        #pragma omp for
        for (size_type i = 0; i < n; ++i)
            p[i] = value_type();
    }
};

template <class T, class U, int N, bool first_touch>
bool operator==(
    aligned_allocator<T, N, first_touch> const &,
    aligned_allocator<U, N, first_touch> const &)
{
    return true;
}

template <class T, class U, int N, bool first_touch>
bool operator!=(
    aligned_allocator<T, N, first_touch> const &,
    aligned_allocator<U, N, first_touch> const &)
{
    return false;
}

template <typename T>
void distribute_pages(
    T * p,
    size_t n)
{
    #pragma omp master
    {
        int num_cpus = omp_get_num_threads();
        int page_size = numa_pagesize();
        int elements_per_page = page_size / sizeof(T);
        int num_pages = (sizeof(T) * n + (page_size - 1)) / page_size;
        int pages_per_cpu = (num_pages + num_cpus - 1) / num_cpus;

        void ** pages = (void **) malloc(
            num_pages * (sizeof(void *) + sizeof(int) + sizeof(int)));
        int * nodes = (int *)(pages + num_pages);
        int * status = (int *)(nodes + num_pages);
        if (!pages)
            throw std::system_error(errno, std::generic_category());

        for (int cpu = 0; cpu < num_cpus; cpu++) {
            int cpu_start_page = std::min(num_pages, cpu * pages_per_cpu);
            int cpu_end_page = std::min(num_pages, (cpu + 1) * pages_per_cpu);
            int node = numa_node_of_cpu(cpu);
            for (int page = cpu_start_page; page < cpu_end_page; page++) {
                pages[page] = p + page * elements_per_page;
                nodes[page] = node;
                status[page] = 0;
            }
        }

        int err = numa_move_pages(
            0, num_pages, pages, nodes, status, MPOL_MF_MOVE);
        if (err < 0) {
            free(pages);
            throw std::system_error(errno, std::generic_category());
        }

        for (int cpu = 0; cpu < num_cpus; cpu++) {
            int cpu_start_page = std::min(num_pages, cpu * pages_per_cpu);
            int cpu_end_page = std::min(num_pages, (cpu + 1) * pages_per_cpu);
            int node = numa_node_of_cpu(cpu);
            for (int page = cpu_start_page; page < cpu_end_page; page++) {
                if (status[page] != node) {
                    std::cerr << "distribute_pages: "
                              << "cpu " << cpu << ", "
                              << "page " << page << ", "
                              << "numa node " << node << ", "
                              << "status " << status << ": "
                              << strerror(-status[page])
                              << '\n';
                }
            }
        }

        free(pages);
    }
}

template <typename T>
int numa_domain_of_index(
    int index,
    int num_indices)
{
    int num_cpus = omp_get_num_threads();
    int page_size = numa_pagesize();
    int num_indices_per_page = page_size / sizeof(T);
    int num_pages = (sizeof(T) * num_indices + (page_size - 1)) / page_size;
    int num_pages_per_cpu = (num_pages + num_cpus - 1) / num_cpus;
    for (int cpu = 0 ; cpu < num_cpus; cpu++) {
        int cpu_page_start = std::min(num_pages, cpu * num_pages_per_cpu);
        int cpu_page_end = std::min(num_pages, (cpu + 1) * num_pages_per_cpu);
        int cpu_index_start = cpu_page_start * num_indices_per_page;
        int cpu_index_end = cpu_page_end * num_indices_per_page;
        if (index >= cpu_index_start && index < cpu_index_end)
            return cpu;
    }
    return num_cpus - 1;
}

#endif
