#ifndef CIRCULAR_BUFFER_HPP
#define CIRCULAR_BUFFER_HPP

#include <algorithm>
#include <vector>

template <typename T>
class CircularBuffer
{

    typedef typename std::vector<T>::value_type value_type;
    typedef typename std::vector<T>::size_type size_type;
    typedef typename std::vector<T>::difference_type difference_type;
    typedef typename std::vector<T>::reference reference;
    typedef typename std::vector<T>::const_reference const_reference;
    typedef typename std::vector<T>::pointer pointer;
    typedef typename std::vector<T>::const_pointer const_pointer;
    typedef typename std::vector<T>::iterator iterator;
    typedef typename std::vector<T>::const_iterator const_iterator;
    typedef typename std::vector<T>::reverse_iterator reverse_iterator;
    typedef typename std::vector<T>::const_reverse_iterator const_reverse_iterator;

public:
    CircularBuffer(size_type capacity)
        : v(capacity)
        , head(0u)
        , tail(0u)
    {
    }

    ~CircularBuffer()
    {
    }

    bool empty() const noexcept
    {
        return head == tail;
    }

    size_type size() const noexcept
    {
        return tail - head;
    }

    reference front()
    {
        return v[head];
    }

    const_reference front() const
    {
        return v[head];
    }

    reference back()
    {
        return v[tail-1u];
    }

    const_reference back() const
    {
        return v[tail-1u];
    }

    template <class... Args>
    reference emplace_back(Args &&... args)
    {
        if (tail == v.size()) {
            auto count = std::max(1ul, head);
            std::rotate(std::begin(v), std::begin(v) + count, std::end(v));
            tail = tail - count;
            head = 0ul;
        }

        v[tail] = T(args...);
        tail++;
        return v[tail - 1u];
    }

    void push_back(T const & value)
    {
        if (tail == v.size()) {
            auto count = std::max(1ul, head);
            std::rotate(std::begin(v), std::begin(v) + count, std::end(v));
            tail = tail - count;
            head = 0ul;
        }

        v[tail] = value;
        tail++;
    }

    void pop_front()
    {
        head++;
    }

    iterator begin() noexcept
    {
        return std::begin(v) + head;
    }

    const_iterator begin() const noexcept
    {
        return std::begin(v) + head;
    }

    const_iterator cbegin() const noexcept
    {
        return std::cbegin(v) + head;
    }

    iterator end() noexcept
    {
        return std::begin(v) + tail;
    }

    const_iterator end() const noexcept
    {
        return std::begin(v) + tail;
    }

    const_iterator cend() const noexcept
    {
        return std::cbegin(v) + tail;
    }

    reverse_iterator rbegin() noexcept
    {
        return std::rbegin(v) + (v.size() - tail);
    }

    const_reverse_iterator rbegin() const noexcept
    {
        return std::rbegin(v) + (v.size() - tail);
    }

    const_reverse_iterator crbegin() const noexcept
    {
        return std::crbegin(v) + (v.size() - tail);
    }

    reverse_iterator rend() noexcept
    {
        return std::rbegin(v) + (v.size() - head);
    }

    const_reverse_iterator rend() const noexcept
    {
        return std::rbegin(v) + (v.size() - head);
    }

    const_reverse_iterator crend() const noexcept
    {
        return std::crbegin(v) + (v.size() - head);
    }

private:
    std::vector<T> v;
    size_type head;
    size_type tail;
};

#endif
