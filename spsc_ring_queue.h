#pragma once

#include <atomic>
#include <cassert>

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
	template<class _Ty>
	struct spsc_ring_queue
	{
		using value_type = _Ty;
		using size_type = size_t;
	public:
		spsc_ring_queue(size_type count)
			: m_bufferPtr((value_type*)::malloc(count * sizeof(value_type)))
			, m_bufferSize(count)
			, m_writeIndex(0)
			, m_readIndex(0)
		{
			//2的幂次方
			assert((count& (count - 1)) == 0 && (count & ~(count - 1)) == count);
		}
		~spsc_ring_queue()
		{
			::free(m_bufferPtr);
		}

		spsc_ring_queue(const spsc_ring_queue&) = delete;
		spsc_ring_queue(spsc_ring_queue&&) = delete;
		spsc_ring_queue& operator =(const spsc_ring_queue&) = delete;
		spsc_ring_queue& operator =(spsc_ring_queue&&) = delete;

        template<class... _Args>
        bool try_push(_Args&&... args);
		bool try_pop(value_type& value);

		template <typename Functor>
		size_t consume_one(Functor const& functor);
		template <typename Functor>
		size_t consume_all(Functor const& functor);

        auto size() const noexcept->size_type;
        auto stride() const noexcept->size_type;
		auto capacity() const noexcept ->size_type;
		bool empty() const noexcept;
        bool full() const noexcept;
	private:
		value_type* m_bufferPtr;
		size_type m_bufferSize;					//必须是2的幂次方

		static const int padding_size = BOOST_LOCKFREE_CACHELINE_BYTES - sizeof(size_type);
		std::atomic<size_type> m_writeIndex;
		char padding1[padding_size]; /* force read_index and write_index to different cache lines */
		std::atomic<size_type> m_readIndex;

        static size_t read_available(size_t write_index, size_t read_index, size_t max_size);
        static size_t write_available(size_t write_index, size_t read_index, size_t max_size);
        size_t read_available(size_t max_size) const;
        size_t write_available(size_t max_size) const;

		auto nextIndex(size_type a_count) const noexcept->size_type;

		template<class Functor>
		static void run_functor_and_delete(value_type* first, value_type* last, Functor const& functor)
		{
			for (; first != last; ++first)
			{
				functor(*first);
				first->~value_type();
			}
		}
	};

	template<class _Ty>
	inline auto spsc_ring_queue<_Ty>::nextIndex(size_type a_count) const noexcept->size_type
	{
		return static_cast<size_type>((a_count + 1) & (m_bufferSize - 1));
	}

    template<class _Ty>
	inline auto spsc_ring_queue<_Ty>::size() const noexcept->size_type
	{
        return read_available(m_bufferSize);
	}
    
    template<class _Ty>
	auto spsc_ring_queue<_Ty>::stride() const noexcept->size_type
	{
		return sizeof(_Ty);
	}

	template<class _Ty>
	inline auto spsc_ring_queue<_Ty>::capacity() const noexcept->size_type
	{
		return m_bufferSize;
	}

	template<class _Ty>
	inline bool spsc_ring_queue<_Ty>::empty() const noexcept
	{
		return read_available(m_bufferSize) == 0;
	}

    template<class _Ty>
	inline bool spsc_ring_queue<_Ty>::full() const noexcept
	{
		return write_available(m_bufferSize) == 0;
	}

	template<class _Ty>
	inline size_t spsc_ring_queue<_Ty>::read_available(size_t write_index, size_t read_index, size_t max_size)
	{
		if (write_index >= read_index)
			return write_index - read_index;
		const size_t ret = write_index + max_size - read_index;
		return ret;
	}

	template<class _Ty>
	inline size_t spsc_ring_queue<_Ty>::write_available(size_t write_index, size_t read_index, size_t max_size)
	{
		size_t ret = read_index - write_index - 1;
		if (write_index >= read_index)
			ret += max_size;
		return ret;
	}

	template<class _Ty>
	inline size_t spsc_ring_queue<_Ty>::read_available(size_t max_size) const
	{
		size_t write_index = m_writeIndex.load(std::memory_order_acquire);
		const size_t read_index = m_readIndex.load(std::memory_order_relaxed);
		return read_available(write_index, read_index, max_size);
	}

	template<class _Ty>
	inline size_t spsc_ring_queue<_Ty>::write_available(size_t max_size) const
	{
		size_t write_index = m_writeIndex.load(std::memory_order_relaxed);
		const size_t read_index = m_readIndex.load(std::memory_order_acquire);
		return write_available(write_index, read_index, max_size);
	}

	template<class _Ty>
    template<class... _Args>
	inline bool spsc_ring_queue<_Ty>::try_push(_Args&&... args)
	{
		const auto write_index = m_writeIndex.load(std::memory_order_acquire);
		const auto next = nextIndex(write_index);
		if (next == m_readIndex.load(std::memory_order_acquire))
			return false;

		new(m_bufferPtr + write_index) value_type(std::forward<_Args>(args)...);

		m_writeIndex.store(next, std::memory_order_release);

		return true;
	}

	template<class _Ty>
	inline bool spsc_ring_queue<_Ty>::try_pop(value_type& value)
	{
		return consume_one([&](value_type&& vt){ value = std::move(vt); }) > 0;
	}

	template<class _Ty>
	template <typename Functor>
	inline size_t spsc_ring_queue<_Ty>::consume_one(Functor const& functor)
	{
		const size_type write_index = m_writeIndex.load(std::memory_order_acquire);
		const size_type read_index = m_readIndex.load(std::memory_order_relaxed); // only written from pop thread
		if (write_index == read_index)
			return 0;

		value_type& object_to_consume = *(m_bufferPtr + read_index);
		functor(std::move(object_to_consume));
		//object_to_consume.~value_type();

		size_type next = nextIndex(read_index);
		m_readIndex.store(next, std::memory_order_release);

		return 1;
	}

	template<class _Ty>
	template <typename Functor>
	inline size_t spsc_ring_queue<_Ty>::consume_all(Functor const& functor)
	{
		const size_t write_index = m_writeIndex.load(std::memory_order_acquire);
		const size_t read_index = m_readIndex.load(std::memory_order_relaxed); // only written from pop thread

		const size_t avail = read_available(write_index, read_index, m_bufferSize);
		if (avail == 0)
			return 0;

		const size_t output_count = avail;

		size_t new_read_index = read_index + output_count;

		if (read_index + output_count > m_bufferSize)
		{
			/* copy data in two sections */
			const size_t count0 = m_bufferSize - read_index;
			const size_t count1 = output_count - count0;

			run_functor_and_delete(m_bufferPtr + read_index, m_bufferPtr + m_bufferSize, functor);
			run_functor_and_delete(m_bufferPtr, m_bufferPtr + count1, functor);

			new_read_index -= m_bufferSize;
		}
		else
		{
			run_functor_and_delete(m_bufferPtr + read_index, m_bufferPtr + read_index + output_count, functor);

			if (new_read_index == m_bufferSize)
				new_read_index = 0;
		}

		m_readIndex.store(new_read_index, std::memory_order_release);
		return output_count;
	}
}
