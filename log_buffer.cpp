#include "log_buffer.h"
#include <cassert>
#include <array>

namespace shark_log
{
    log_buffer::log_buffer(size_type count, size_type chunk)
        : m_bufferPtr((char*)::malloc(count* ((size_t)1u << chunk)))
        , m_bufferSize(count)
        , m_chunkIndex(chunk)
        , m_writeIndex(0)
        , m_readIndex(0)
    {
        //2的幂次方
        assert((count & (count - 1)) == 0 && (count & ~(count - 1)) == count);
    }

    log_buffer::~log_buffer()
    {
        ::free(m_bufferPtr);
    }

    extern __time64_t _log_tick_2_time(uint64_t tick);

    static inline void write_to_binary(log_info_base* log, log_file* file)
    {
        const log_type* type = log->type;

        __time64_t t64 = _log_tick_2_time(log->tick);
        if (file->write(&type->tid, sizeof(type->tid)) && file->write(&t64, sizeof(t64)))
            type->serialize(log, file);
        else
            type->serialize(log, nullptr);      //阻止写入文件，但又需要清理数据
    }

    static inline void write_to_text(log_info_base* log, log_file* file)
    {
        const log_type* type = log->type;

        __time64_t t64 = _log_tick_2_time(log->tick);
        __time64_t tus = t64 % 1000000;
        t64 /= 1000000;

        __time64_t tms = tus / 1000;
        tus = tus % 1000;

        struct tm tl;
        localtime_s(&tl, &t64);

        std::string text = fmt::format("{0:04d}-{1:02d}-{2:02d} {3:02d}:{4:02d}:{5:02d} {6:03d},{7:03d}>{8}({9}): {10}:\r\n",
            tl.tm_year + 1900, tl.tm_mon + 1, tl.tm_mday, tl.tm_hour, tl.tm_min, tl.tm_sec, tms, tus,
            type->file, type->line, _log_level_string(type->level));

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

    static inline void write_to_text_tick(log_info_base* log, log_file* file)
    {
        const log_type* type = log->type;
        std::string text = fmt::format("{0}>{1}({2}): {3}:\r\n",
            log->tick, type->file, type->line, _log_level_string(type->level));

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

    size_t log_buffer::consume_one(log_file* file, bool binaryMode)
    {
        const size_type write_index = m_writeIndex.load(std::memory_order_acquire);
        const size_type read_index = m_readIndex.load(std::memory_order_relaxed); // only written from pop thread
        if (write_index == read_index)
            return 0;

        char* buf = m_bufferPtr + ((size_t)read_index << m_chunkIndex);
        log_info_base* log = reinterpret_cast<log_info_base*>(buf);
        const log_type* type = log->type;
        if (type)
        {
            if (binaryMode)
                write_to_binary(log, file);
            else
                write_to_text_tick(log, file);
            log->type = nullptr;
        }

        size_type next = nextIndex(read_index);
        m_readIndex.store(next, std::memory_order_release);

        return 1;
    }

    using size_type = log_buffer::size_type;
    static void run_functor_and_delete(char* first, char* last, size_type stride, log_file* file, bool binaryMode)
    {
        for (; first != last; first += stride)
        {
            log_info_base* log = reinterpret_cast<log_info_base*>(first);
            const log_type* type = log->type;
            if (type)
            {
                if (binaryMode)
                    write_to_binary(log, file);
                else
                    write_to_text_tick(log, file);
                log->type = nullptr;
            }
        }
    }

    size_type log_buffer::consume_all(log_file* file, bool binaryMode)
    {
        const size_type write_index = m_writeIndex.load(std::memory_order_acquire);
        const size_type read_index = m_readIndex.load(std::memory_order_relaxed); // only written from pop thread

        const size_type avail = read_available(write_index, read_index, m_bufferSize);
        if (avail == 0)
            return 0;

        const size_type output_count = avail;
        const size_type stride = (size_type)((size_type)1u << m_chunkIndex);

        size_type new_read_index = read_index + output_count;

        if (read_index + output_count > m_bufferSize)
        {
            /* copy data in two sections */
            const size_type count0 = m_bufferSize - read_index;
            const size_type count1 = output_count - count0;

            run_functor_and_delete(m_bufferPtr + (read_index << m_chunkIndex), m_bufferPtr + (m_bufferSize << m_chunkIndex), stride, file, binaryMode);
            run_functor_and_delete(m_bufferPtr, m_bufferPtr + (count1 << m_chunkIndex), stride, file, binaryMode);

            new_read_index -= m_bufferSize;
        }
        else
        {
            run_functor_and_delete(m_bufferPtr + (read_index << m_chunkIndex), m_bufferPtr + ((read_index + output_count) << m_chunkIndex), stride, file, binaryMode);

            if (new_read_index == m_bufferSize)
                new_read_index = 0;
        }

        m_readIndex.store(new_read_index, std::memory_order_release);
        return output_count;
    }
}
