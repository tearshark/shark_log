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
        //2的幂次方
        assert((count & (count - 1)) == 0 && (count & ~(count - 1)) == count);
    }

    log_buffer::~log_buffer()
    {
        ::free(m_bufferPtr);
    }

    static inline void write_to_binary(log_info_base* log, log_file* file)
    {
		const log_type* type = log->type;
        if (file->write(&type->tid, sizeof(type->tid)) && file->write(&log->tick, sizeof(log->tick)))
		    type->serialize(log, file);
        else
            type->serialize(log, nullptr);      //阻止写入文件，但又需要清理数据
    }

	static inline void write_to_text(log_info_base* log, log_file* file)
	{
		const log_type* type = log->type;

		std::string text = fmt::format("{0}>{1}({2}): {3}:\r\n", log->tick, type->file, type->line, _log_level_string(type->level));
        if (file->write(text.c_str(), text.size()))
        {
            type->formator(log, file);
            file->write("\r\n", 2);
        }
        else
        {
            type->formator(log, nullptr);      //阻止写入文件，但又需要清理数据
        }
	}

    bool log_buffer::try_pop(log_file* file)
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
                write_to_binary(log, file);
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
