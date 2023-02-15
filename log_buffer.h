#pragma once
#include <memory>
#include <tuple>
#include <atomic>
#include <thread>
#include <cassert>

#include "log_info.h"

#if !defined(BOOST_LOCKFREE_CACHELINE_BYTES)
// PowerPC caches support 128-byte cache lines.
#if defined(powerpc) || defined(__powerpc__) || defined(__ppc__)
#define BOOST_LOCKFREE_CACHELINE_BYTES 128
#else
#define BOOST_LOCKFREE_CACHELINE_BYTES 128
#endif
#endif

namespace shark_log
{
    struct log_buffer
    {
        using value_type = char;
        using size_type = size_t;
    public:
        log_buffer(size_type count, size_type chunk);
        ~log_buffer();

        log_buffer(const log_buffer&) = delete;
        log_buffer(log_buffer&&) = delete;
        log_buffer& operator =(const log_buffer&) = delete;
        log_buffer& operator =(log_buffer&&) = delete;

        template<class... _Args>
        size_type try_push(const log_type& s_type, _Args&&... args)
#if _HAS_CXX17 || _HAS_CXX20 || __cplusplus >= 201703L
            noexcept(std::is_nothrow_constructible_v<log_info<log_convert_t<_Args>...>, _Args...>)
#endif
            ;

        size_type consume_one(log_file* file, bool binaryMode);
        size_type consume_all(log_file* file, bool binaryMode);

        static size_type read_available(size_type write_index, size_type read_index, size_type max_size) noexcept;
        static size_type write_available(size_type write_index, size_type read_index, size_type max_size) noexcept;
        size_type read_available(size_type max_size) const noexcept;
        size_type write_available(size_type max_size) const noexcept;
        auto capacity() const noexcept ->size_type;
        inline bool empty() const noexcept;
    private:
        value_type* m_bufferPtr;
        size_type m_bufferSize;					//必须是2的幂次方
        size_type m_chunkIndex;

        static const int padding_size = BOOST_LOCKFREE_CACHELINE_BYTES - sizeof(size_type);
        std::atomic<size_type> m_writeIndex;
        char padding1[padding_size]; /* force read_index and write_index to different cache lines */
        std::atomic<size_type> m_readIndex;

        auto nextIndex(size_type a_count) const noexcept->size_type;
    };

    inline auto log_buffer::nextIndex(size_type a_count) const noexcept->size_type
    {
        return static_cast<size_type>((a_count + 1) & (m_bufferSize - 1));
    }

    inline auto log_buffer::capacity() const noexcept->size_type
    {
        return m_bufferSize;
    }

    inline bool log_buffer::empty() const noexcept
    {
        return read_available(m_bufferSize) == 0;
    }

    inline log_buffer::size_type log_buffer::read_available(size_type write_index, size_type read_index, size_type max_size) noexcept
    {
        if (write_index >= read_index)
            return write_index - read_index;
        const size_type ret = write_index + max_size - read_index;
        return ret;
    }

    inline log_buffer::size_type log_buffer::write_available(size_type write_index, size_type read_index, size_type max_size) noexcept
    {
        size_type ret = read_index - write_index - 1;
        if (write_index >= read_index)
            ret += max_size;
        return ret;
    }

    inline log_buffer::size_type log_buffer::read_available(size_type max_size) const noexcept
    {
        size_type write_index = m_writeIndex.load(std::memory_order_acquire);
        const size_type read_index = m_readIndex.load(std::memory_order_relaxed);
        return read_available(write_index, read_index, max_size);
    }

    inline log_buffer::size_type log_buffer::write_available(size_type max_size) const noexcept
    {
        size_type write_index = m_writeIndex.load(std::memory_order_relaxed);
        const size_type read_index = m_readIndex.load(std::memory_order_acquire);
        return write_available(write_index, read_index, max_size);
    }

    template<class... _Args>
    inline log_buffer::size_type log_buffer::try_push(const log_type& s_type, _Args&&... args)
#if _HAS_CXX17 || _HAS_CXX20 || __cplusplus >= 201703L
    noexcept(std::is_nothrow_constructible_v<log_info<log_convert_t<_Args>...>, _Args...>)
#endif
    {
        using info_t = log_info<log_convert_t<_Args>...>;
        static_assert(sizeof(info_t) <= _LOG_MAX_INFO, "Exceeded the maximum heads-up log limit");

        const auto write_index = m_writeIndex.load(std::memory_order_acquire);
        const auto next = nextIndex(write_index);
        if (next == m_readIndex.load(std::memory_order_acquire))
            return 0;

        char* buf = m_bufferPtr + (write_index << m_chunkIndex);
        info_t* log = new(buf) info_t(&s_type, this, std::forward<_Args>(args)...);
        (void)log;

        m_writeIndex.store(next, std::memory_order_release);

        return read_available(next, m_readIndex.load(std::memory_order_acquire), m_bufferSize);
    }
}
