#ifndef ALIGNED_ALLOCATOR_HPP
#define ALIGNED_ALLOCATOR_HPP

#include <cstddef>
#include <cstdlib>
#include <limits>
#include <new>
#include <system_error>
#include <errno.h>

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
        #pragma omp parallel for
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

#endif
