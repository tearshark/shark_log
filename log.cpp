﻿#include <iostream>
#include <array>
#include <mutex>
#include "semaphore.h"
#include "log.h"

namespace shark_log
{
    struct loop_semaphore
    {
        std::atomic<intptr_t> _counter = 0;
        void release()
        {
            _counter.fetch_add(1, std::memory_order_acq_rel);
        }
        bool acquire()
        {
            for (;;)
            {
                intptr_t cnt = _counter.load(std::memory_order_relaxed);
                if (cnt > 0)
                {
                    if (_counter.compare_exchange_weak(cnt, cnt - 1))
                        break;
                }
            }
            return true;
        }
    };

    log_level g_log_level = log_level::info;
    int g_log_min_format_interval = 0;
    std::atomic<uint16_t> g_log_tid_counter{ 0 };

    static __time64_t g_log_start_clock;    //微秒
    static uint64_t g_log_start_tick = 0;
    static uint64_t g_log_tick_freq = 1000;

    static log_file_factory* g_log_factor = nullptr;
    static std::string g_log_root;

    static volatile bool g_log_exit = false;
    static std::thread g_log_thread;
    static std::semaphore g_log_notify;
    //static loop_semaphore g_log_notify;

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
            for (;;)
            {
                bool all_empty = true;
                for (uint32_t i = 0; i < (uint32_t)buffers.size(); ++i)
                {
                    auto* logb = buffers[i].get();
                    all_empty &= logb->empty();
                }

                if (!all_empty)
                {
                    g_log_notify.release();
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                }
                else
                {
                    break;
                }
            }

            std::unique_lock<std::mutex> lck(g_mng_mutex);

            if (next)
                next->prev = prev;
            *prev = this->next;
        }

        void try_consume_all(log_file* file, bool binaryMode)
        {
            for (uint32_t i = 0; i < (uint32_t)buffers.size(); ++i)
            {
                auto* logb = buffers[i].get();
                logb->consume_all(file, binaryMode);
            }
        }
    };

    thread_local log_buffer_mng g_mng;

    log_buffer* shark_log_local_buffer(size_t idx)
    {
        assert(idx < _LOG_BUFFER_CNT);
        return g_mng.buffers[idx].get();
    }

    static std::unique_ptr<log_file> create_log_file(bool writeMode, bool binaryMode)
    {
        time_t t = time(nullptr);
        struct tm tmnow;
        localtime_s(&tmnow, &t);

        const char* ext = binaryMode ? "bin" : "txt";
        std::string path = fmt::format(g_log_root, tmnow.tm_year + 1900, tmnow.tm_mon + 1, tmnow.tm_mday, tmnow.tm_hour, tmnow.tm_min, tmnow.tm_sec, ext);
        log_file* file = g_log_factor->create(path, writeMode);
        if (file == nullptr)
            fmt::print("open file '{}' failed.", path);

        return std::unique_ptr<log_file>(file);
    }

    static void shark_log_loop_all_mng(log_file* file, bool binaryMode)
    {
        auto id = std::this_thread::get_id();

        std::unique_lock<std::mutex> lck(g_mng_mutex);

        for (log_buffer_mng* mng = g_mng_first; mng != nullptr; mng = mng->next)
        {
            if (mng->id != id)
                mng->try_consume_all(file, binaryMode);
        }
    }

    static void shark_log_loop_format(bool binaryMode)
    {
        //SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
        //SetThreadAffinityMask(GetCurrentThread(), 1);

        try
        {
            auto file = create_log_file(true, binaryMode);

            for (; !g_log_exit;)
            {
                g_log_notify.try_acquire_for(std::chrono::milliseconds(g_log_min_format_interval));   //每100ms强制落地一次数据
                //if (!g_log_notify.acquire())
                //    break;
                shark_log_loop_all_mng(file.get(), binaryMode);
            }

            shark_log_loop_all_mng(file.get(), binaryMode);
        }
        catch (std::exception& e)
        {
            std::cout << e.what() << std::endl;
        }
    }

    void shark_log_initialize(log_file_factory* factor, bool binaryMode, std::string root)
    {
        assert(factor != nullptr);
        g_log_exit = false;

        g_log_start_clock = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) * 1000000;
        g_log_start_tick = _log_tick();
        g_log_tick_freq = _log_tick_freq();

        g_log_factor = factor;
        g_log_root = std::move(root);

        g_log_thread = std::thread(&shark_log_loop_format, binaryMode);
    }

    __time64_t _log_tick_2_time(uint64_t tick)
    {
        return g_log_start_clock + (tick - g_log_start_tick) / g_log_tick_freq;
    }

    void shark_log_destroy()
    {
        g_log_exit = true;
        g_log_notify.release();

        g_log_thread.join();
    }

    void shark_log_notify_format(log_buffer* logb)
    {
        g_log_notify.release();
    }


}
