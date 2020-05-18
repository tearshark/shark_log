#include <array>
#include <mutex>
#include "semaphore.h"
#include "log.h"

namespace shark_log
{
    log_level g_log_level = log_level::info;
    std::atomic<uint16_t> g_log_tid_counter{ 0 };
    
    static log_file_factory* g_log_factor = nullptr;
    static std::string g_log_root;

    static volatile bool g_log_exit = false;
    static std::thread g_log_thread;
    static std::semaphore g_log_notify;

    struct log_buffer_mng;

    static std::mutex g_mng_mutex;
    static log_buffer_mng* g_mng_first = nullptr;

    struct log_buffer_mng
    {
        using buffer_uptr = std::unique_ptr<log_buffer>;

        log_buffer_mng* next = nullptr;
        log_buffer_mng** prev = nullptr;

        std::thread::id id;
        std::array<buffer_uptr, _LOG_BUFFER_CNT> buffers;

        log_buffer_mng()
        {
            id = std::this_thread::get_id();

            for (uint32_t i = 0; i < (uint32_t)buffers.size(); ++i)
            {
                uint32_t chunk = (uint32_t)(_LOG_ALIGN_REQ << i);
                uint32_t count = _LOG_CACHE_SIZE / chunk;
                buffers[i] = buffer_uptr(new log_buffer(count, _LOG_ALIGN_LSHIFT + i));
            }

            std::unique_lock<std::mutex> lck(g_mng_mutex);

            if (g_mng_first != nullptr)
                g_mng_first->prev = &next;
            next = g_mng_first;
            prev = &g_mng_first;
            g_mng_first = this;
        }

        ~log_buffer_mng() noexcept(false)
        {
            std::unique_lock<std::mutex> lck(g_mng_mutex);

            if (next)
                next->prev = prev;
            *prev = this->next;
        }

        void try_pop_all(log_file* file)
        {
            for (uint32_t i = 0; i < (uint32_t)buffers.size(); ++i)
            {
                auto* logb = buffers[i].get();
                logb->consume_all(file);
            }
        }
    };

    thread_local log_buffer_mng g_mng;

    log_buffer* shark_log_local_buffer(size_t idx)
    {
        assert(idx < _LOG_BUFFER_CNT);
        return g_mng.buffers[idx].get();
    }

    static std::unique_ptr<log_file> create_log_file(bool writeMode)
    {
        time_t t = time(nullptr);
        struct tm tmnow;
        localtime_s(&tmnow, &t);

        std::string path = fmt::format(g_log_root, tmnow.tm_year + 1900, tmnow.tm_mon + 1, tmnow.tm_mday, tmnow.tm_hour, tmnow.tm_min, tmnow.tm_sec);
        log_file* file = g_log_factor->create(path, writeMode);

        return std::unique_ptr<log_file>(file);
    }

    static void shark_log_loop_all_mng(log_file* file)
    {
        auto id = std::this_thread::get_id();

        std::unique_lock<std::mutex> lck(g_mng_mutex);

        for (log_buffer_mng* mng = g_mng_first; mng != nullptr; mng = mng->next)
        {
            if (mng->id != id)
                mng->try_pop_all(file);
        }
    }

    static void shark_log_loop_format()
    {
        auto file = create_log_file(true);

        for(;!g_log_exit;)
        {
            g_log_notify.try_acquire_for(std::chrono::milliseconds(100));
            shark_log_loop_all_mng(file.get());
        }

        shark_log_loop_all_mng(file.get());
    }

    void shark_log_initialize(log_file_factory* factor, std::string root)
    {
        assert(factor != nullptr);
        g_log_factor = factor;
        g_log_root = std::move(root);

        g_log_thread = std::thread(&shark_log_loop_format);
    }

    void shark_log_destroy()
    {
        g_log_exit = true;
        g_log_notify.release();

        g_log_thread.join();
    }
    
    void shark_log_notify_format(log_buffer* logb)
    {
        //g_log_notify.release();
    }
}
