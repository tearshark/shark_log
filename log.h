#pragma once
#include "log_info.h"
#include "log_buffer.h"
#include "log_file.h"

namespace shark_log
{
    extern log_level g_log_level;
    extern std::atomic<uint16_t> g_log_tid_counter;

    extern log_buffer* shark_log_local_buffer(size_t idx);

    //初始化日志系统
    //factor : 创建文件的工厂
    //root : 格式化日志文件名的字符串，内部会提供创建的时间，用{fmt}格式化出文件名。
    //      如 "C:/log/mylog-{%04d}{%02d}{%02d}-{%02d}{%02d}{%02d}.txt"
    extern void shark_log_initialize(log_file_factory* factor, std::string root);
    extern void shark_log_destroy();
    extern void shark_log_notify_format(log_buffer* logb);

    template<log_level level, class... _Args>
    inline void _shark_log_push(const log_type& s_type, _Args&&... args)
    {
        if (level >= g_log_level)
        {
            using info_t = log_info<log_convert_t<_Args>...>;

			log_buffer* logb = shark_log_local_buffer(_log_align_idx<info_t>());

            size_t count;
            if ((count = logb->try_push<level>(s_type, std::forward<_Args>(args)...)) == 0)
            {
                for(int i = 16; i > 0; --i)
                {
                    std::this_thread::yield();
                    if ((count = logb->try_push<level>(s_type, std::forward<_Args>(args)...)) > 0)
                        goto push_success;
                }

                for(int i = 0; i < 32; ++i)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(i));
                    if ((count = logb->try_push<level>(s_type, std::forward<_Args>(args)...)) > 0)
                        goto push_success;
                }
            }
        push_success:;
/*
            if (count >= (logb->capacity() >> 1))
            {
                shark_log_notify_format(logb);
            }
*/
        }
    }

#define _shark_log_with_type(file, line, level, str, ...) \
    do\
    {\
        using info_t = decltype(shark_decval(__VA_ARGS__));\
        static const log_type s_type =\
        {\
            &info_t::format_and_destructor,\
            &info_t::serialize_and_destructor,\
            &info_t::translate_to_text,\
            str,\
            file,\
            static_cast<uint32_t>(line),\
            ++g_log_tid_counter,\
            level\
        };\
        _shark_log_push<level>(s_type, __VA_ARGS__);\
    }while(false)

#define sharkl_info(str, ...) _shark_log_with_type(__FILE__, __LINE__, log_level::info, str, __VA_ARGS__)
#define sharkl_warn(str, ...) _shark_log_with_type(__FILE__, __LINE__, log_level::warn, str, __VA_ARGS__)
#define sharkl_eror(str, ...) _shark_log_with_type(__FILE__, __LINE__, log_level::eror, str, __VA_ARGS__)
}
