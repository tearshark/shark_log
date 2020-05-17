#pragma once
#define FMT_HEADER_ONLY
#include <fmt/format.h>

#include "log_type.h"

namespace shark_log
{
    template <typename S, class _Tuple, size_t... _Indices>
    void format_print_impl(std::FILE* f, const S& format_str, _Tuple&& _Tpl, std::index_sequence<_Indices...>)
    {
        fmt::print(f, format_str, std::get<_Indices>(std::forward<_Tuple>(_Tpl))...);
    }
    template <typename S, class _Tuple, size_t... _Indices>
    std::string format_print_impl(const S& format_str, _Tuple&& _Tpl, std::index_sequence<_Indices...>)
    {
        return fmt::format(format_str, std::get<_Indices>(std::forward<_Tuple>(_Tpl))...);
    }

    struct log_buffer;

    struct log_info_base
    {
        const log_type* type;
        log_buffer* buffer;
        uint64_t tick;

        log_info_base(const log_type* t, log_buffer* b)
            : type(t)
            , buffer(b)
            , tick(_log_tick())
        {}

        log_info_base(const log_info_base&) = default;
        log_info_base& operator =(const log_info_base&) = default;
    };

    template<class... _Args>
    struct log_info : log_info_base
    {
        using this_type = log_info<_Args...>;
        using tuple_type = std::tuple<_Args...>;
        using read_type = std::tuple<read_type_t<_Args>...>;

        tuple_type args;

        template<class... U>
        log_info(const log_type* t, log_buffer* b, U&&... parames)
            : log_info_base(t, b)
            , args(std::forward<U>(parames)...)
        {}
        log_info(const log_info&) = default;
        log_info& operator =(const log_info&) = default;

        static bool serialize_and_destructor(void* pvthiz, log_file& file)
        {
            this_type* thiz = reinterpret_cast<this_type*>(pvthiz);
            bool result = log_serialize_to_file(file, thiz->args);
            thiz->~log_info();

            return result;
        }

        static bool translate_to_text(log_type& type, log_file& source, log_file& target)
        {
            read_type value = log_serialize_from_file<read_type>(source);
            std::string text = format_print_impl(type.str, value, std::index_sequence_for<_Args...>{});

            if (!target.write(text.c_str(), text.size()))
                return false;
            if (!target.write("\r\n", 2))
                return false;
            return true;
        }

        static void format_and_destructor(void* pvthiz, log_file& file)
        {
            this_type* thiz = reinterpret_cast<this_type*>(pvthiz);
            //const log_type& type = *thiz->type;

            //fmt::print(stdout, "{0}>{1}({2}): {3}:\n", thiz->tick, type.file, type.line, _log_level_string(type.level));
            //format_print_impl(stdout, type.str, std::move(thiz->args), std::index_sequence_for<_Args...>{});
            //fmt::print(stdout, "\n");

            thiz->~log_info();
        }
    };

    template<class... _Args>
    auto shark_decval(_Args&&... args)->log_info<log_convert_t<_Args>...>;
}
