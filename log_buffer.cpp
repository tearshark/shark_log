#include "log_buffer.h"
#include <cassert>
#include <array>

namespace shark_log
{
    log_buffer::log_buffer(size_type count, size_type chunk)
        : m_bufferPtr((char *)::malloc(count * ((size_t)1u << chunk)))
        , m_bufferSize(count)
        , m_chunkIndex(chunk)
        , m_writeIndex(0)
        , m_readIndex(0)
        , m_maximumReadIndex(0)
        , m_count(0)
    {
        //2µÄÃÝ´Î·½
        assert((count & (count - 1)) == 0 && (count & ~(count - 1)) == count);
    }

    log_buffer::~log_buffer()
    {
        ::free(m_bufferPtr);
    }

    bool log_buffer::try_pop()
    {
        auto currentReadIndex = m_readIndex.load(std::memory_order_acquire);

        for (;;)
        {
            auto idx = countToIndex(currentReadIndex);
            if (idx == countToIndex(m_maximumReadIndex.load(std::memory_order_acquire)))
                return false;

            char* buf = m_bufferPtr + (idx << m_chunkIndex);
            log_info_base* log = reinterpret_cast<log_info_base*>(buf);
            const log_type* type = log->type;
            if (type)
            {
                type->formator(log);
                log->type = nullptr;
            }

            if (m_readIndex.compare_exchange_strong(currentReadIndex, nextIndex(currentReadIndex), std::memory_order_acq_rel))
            {
                --m_count;
                return true;
            }
        }
    }
}
