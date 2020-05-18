#pragma once
#include <memory>
#include <tuple>
#include <atomic>
#include <thread>
#include <cassert>

#include "log_info.h"

namespace shark_log
{
	struct log_buffer
	{
		using size_type = uint32_t;
	public:
		log_buffer(size_type count, size_type chunk);
		~log_buffer();

		log_buffer(const log_buffer&) = delete;
		log_buffer(log_buffer&&) = delete;
		log_buffer& operator =(const log_buffer&) = delete;
		log_buffer& operator =(log_buffer&&) = delete;

		bool empty() const noexcept;
		bool full() const noexcept;
		auto capacity() const noexcept ->size_type;

		template<log_level level, class... _Args>
		auto try_push(const log_type& s_type, _Args&&... args) ->size_type;

		bool try_pop(log_file* file);
	private:
		char* m_bufferPtr;
		size_type m_bufferSize;					//必须是2的幂次方
		size_type m_chunkIndex;

		std::atomic<size_type> m_writeIndex;
		std::atomic<size_type> m_readIndex;
		std::atomic<size_type> m_maximumReadIndex;

		std::atomic<size_type> m_count;

		auto countToIndex(size_type a_count) const noexcept->size_type;
		auto nextIndex(size_type a_count) const noexcept->size_type;
	};

	inline auto log_buffer::countToIndex(size_type a_count) const noexcept->size_type
	{
		return (a_count & (m_bufferSize - 1));
	}

	inline auto log_buffer::nextIndex(size_type a_count) const noexcept->size_type
	{
		return static_cast<size_type>((a_count + 1));
	}

	inline bool log_buffer::empty() const noexcept
	{
		auto currentWriteIndex = m_maximumReadIndex.load(std::memory_order_acquire);
		auto currentReadIndex = m_readIndex.load(std::memory_order_acquire);
		return countToIndex(currentWriteIndex) == countToIndex(currentReadIndex);
	}

	inline bool log_buffer::full() const noexcept
	{
		auto currentWriteIndex = m_writeIndex.load(std::memory_order_acquire);
		auto currentReadIndex = m_readIndex.load(std::memory_order_acquire);
		return countToIndex(nextIndex(currentWriteIndex)) == countToIndex(currentReadIndex);
	}

	inline auto log_buffer::capacity() const noexcept->size_type
	{
		return m_bufferSize;
	}

	template<log_level level, class... _Args>
	inline auto log_buffer::try_push(const log_type& s_type, _Args&&... args) ->size_type
	{
		using info_t = log_info<log_convert_t<_Args>...>;
		static_assert(sizeof(info_t) <= _LOG_MAX_INFO, "Exceeded the maximum heads-up log limit");

		auto currentWriteIndex = m_writeIndex.load(std::memory_order_acquire);

		do
		{
			if (countToIndex(nextIndex(currentWriteIndex)) == countToIndex(m_readIndex.load(std::memory_order_acquire)))
				return 0;
		} while (!m_writeIndex.compare_exchange_strong(currentWriteIndex, nextIndex(currentWriteIndex), std::memory_order_acq_rel));

		char* buf = m_bufferPtr + (countToIndex(currentWriteIndex) << m_chunkIndex);
		info_t* log = new(buf) info_t(&s_type, this, std::forward<_Args>(args)...);
		(void)log;

		auto savedWriteIndex = currentWriteIndex;
		while (!m_maximumReadIndex.compare_exchange_weak(currentWriteIndex, nextIndex(currentWriteIndex), std::memory_order_acq_rel))
		{
			currentWriteIndex = savedWriteIndex;
			std::this_thread::yield();
		}

		size_type count = ++m_count;
		(void)count;

		return count;
	}
}
