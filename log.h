#pragma once
#include "log_info.h"
#include "log_buffer.h"

namespace shark_log
{
    extern log_level g_log_level;

    extern log_buffer* shark_log_local_buffer(size_t idx);
    extern void shark_log_initialize();
    extern void shark_log_destroy();
    extern void shark_log_notify_format(log_buffer* logb);

    template<log_level level, class... _Args>
    inline void _shark_log_push(const log_type& s_type, _Args&&... args)
    {
        if (level >= g_log_level)
        {
            using info_t = log_info<log_convert_t<_Args>...>;

            log_buffer* logb = shark_log_local_buffer(_log_align_idx<info_t>());
            uint32_t count;
            if (count = logb->try_push<level>(s_type, std::forward<_Args>(args)...); count == 0)
            {
                for(int i = 16; i > 0; --i)
                {
                    std::this_thread::yield();
                    if (count = logb->try_push<level>(s_type, std::forward<_Args>(args)...); count > 0)
                        goto push_success;
                }

                for(int i = 0; i < 32; ++i)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(i));
                    if (count = logb->try_push<level>(s_type, std::forward<_Args>(args)...); count > 0)
                        goto push_success;
                }
            }
        push_success:
            if (count >= (logb->capacity() >> 1))
            {
                shark_log_notify_format(logb);
            }
        }
    }

#define _shark_log_with_type(file, line, level, str, ...) \
    do\
    {\
        using info_t = decltype(shark_decval(__VA_ARGS__));\
        static const log_type s_type =\
        {\
            &info_t::format_and_destructor,\
            str,\
            file,\
            static_cast<uint32_t>(line),\
            level\
        };\
        _shark_log_push<level>(s_type, __VA_ARGS__);\
    }while(false)

#define sharkl_info(str, ...) _shark_log_with_type(__FILE__, __LINE__, log_level::info, str, __VA_ARGS__)
#define sharkl_warn(str, ...) _shark_log_with_type(__FILE__, __LINE__, log_level::warn, str, __VA_ARGS__)
#define sharkl_eror(str, ...) _shark_log_with_type(__FILE__, __LINE__, log_level::eror, str, __VA_ARGS__)
}
