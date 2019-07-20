#ifndef ALIGNED_ALLOCATOR_HPP
#define ALIGNED_ALLOCATOR_HPP

#include <omp.h>
#include <numa.h>
#include <numaif.h>

#include <cassert>
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

static inline void * align_upwards(void const * p, uintptr_t alignment)
{
    assert(alignment > 0 && (alignment & (alignment - 1)) == 0);
    assert(p != 0);
    uintptr_t address = (uintptr_t) p;
    if (address % alignment != 0)
        address += alignment - address % alignment;
    assert(address >= (uintptr_t) p);
    return (void *) address;
}

static inline void * align_downwards(void const * p, uintptr_t alignment)
{
    assert(alignment > 0 && (alignment & (alignment - 1)) == 0);
    assert(p != 0);
    uintptr_t address = (uintptr_t) p;
    address -= address % alignment;
    assert(address <= (uintptr_t) p);
    return (void *) address;
}

template <typename T>
int cpu_of_page(
    T const * p,
    size_t num_elements,
    int num_cpus,
    int page)
{
    int page_size = numa_pagesize();
    uint8_t * start_address = (uint8_t *) align_downwards(p, page_size);
    int num_elements_per_cpu = (num_elements + num_cpus - 1) / num_cpus;
    uint8_t * page_address = start_address + page * page_size;
    int cpu = 0;
    for (; cpu < num_cpus; cpu++) {
        int cpu_end_element = std::min<int>(num_elements, (cpu + 1) * num_elements_per_cpu) - 1;
        uint8_t * cpu_end_address = (uint8_t *) (p + cpu_end_element);
        if (cpu_end_address >= page_address)
            return cpu;
    }
    return num_cpus - 1;
}

template <typename T>
int page_of_index(
    T const * p,
    size_t num_elements,
    int index,
    int num_cpus,
    int page_size)
{
    uint8_t * start_address = (uint8_t *) align_downwards(p, page_size);
    uint8_t * end_address = (uint8_t *) (p + num_elements);
    int num_pages = (end_address - start_address + (page_size - 1)) / page_size;
    for (int page = 0; page < num_pages; page++) {
        T * next_page = (T *) align_upwards(p + 1, page_size);
        int page_num_elements = next_page - p;
        if (index < page_num_elements)
            return page;
        index -= page_num_elements;
        p = next_page;
    }
    return num_pages - 1;
}

template <typename T>
int cpu_of_index(
    T const * p,
    size_t num_elements,
    int index,
    int num_cpus,
    int page_size)
{
    int page = page_of_index(p, num_elements, index, num_cpus, page_size);
    return cpu_of_page(p, num_elements, num_cpus, page);
}

template <typename T>
void distribute_pages(
    T const * p,
    size_t n)
{
    #pragma omp master
    {
        int page_size = numa_pagesize();
        uint8_t * start_address = (uint8_t *) align_downwards(p, page_size);
        uint8_t * end_address = (uint8_t *) (p + n);
        int num_pages = (end_address - start_address + (page_size - 1)) / page_size;
        void ** pages = (void **) malloc(
            num_pages * (sizeof(void *) + sizeof(int) + sizeof(int)));
        int * nodes = (int *)(pages + num_pages);
        int * status = (int *)(nodes + num_pages);
        if (!pages)
            throw std::system_error(errno, std::generic_category());

        int num_cpus = omp_get_num_threads();
        for (int page = 0; page < num_pages; page++) {
            uint8_t * page_address = start_address + page * page_size;
            int cpu = cpu_of_page(p, n, num_cpus, page);
            int node = numa_node_of_cpu(cpu);
            pages[page] = (void *) page_address;
            nodes[page] = node;
            status[page] = 0;
            // std::cout << "Moving page " << (void *) page_address << " "
            //           << "(" << page << " of " << num_pages << ") "
            //           << "to CPU " << cpu << ", NUMA domain " << node << '\n';
        }

        int err = numa_move_pages(
            0, num_pages, pages, nodes, status, MPOL_MF_MOVE);
        if (err < 0) {
            free(pages);
            throw std::system_error(errno, std::generic_category());
        }

        for (int page = 0; page < num_pages; page++) {
            uint8_t * page_address = start_address + page * page_size;
            int cpu = cpu_of_page(p, n, num_cpus, page);
            int node = numa_node_of_cpu(cpu);
            if (status[page] != node) {
                std::cerr << "distribute_pages: "
                          << "Failed to move page " << (void *) page_address << " "
                          << "(" << page << " of " << num_pages << ") "
                          << "to CPU " << cpu << ", NUMA domain " << node << ": "
                          << strerror(-status[page]) << '\n';
            }
        }
        free(pages);
    }
}

#endif
