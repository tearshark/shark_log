#pragma once
#include "log_const.h"

namespace shark_log
{
    using log_formator = void(void*);

    struct log_type
    {
        log_formator* formator;
        const char* str;
        const char* file;
        uint32_t line;
        log_level level;
    };

    template<class _Ty>
    struct log_convert
    {
        using type = _Ty;
    };
    template<class _Ty>
    struct log_convert<_Ty&>
    {
        using type = _Ty;
    };
    template<class _Ty>
    struct log_convert<_Ty&&>
    {
        using type = _Ty;
    };
    template<size_t N>
    struct log_convert<const char[N]>
    {
        using type = const char*;
    };
    template<size_t N>
    struct log_convert<const wchar_t[N]>
    {
        using type = const wchar_t*;
    };
    template<size_t N>
    struct log_convert<const char16_t[N]>
    {
        using type = const char16_t*;
    };
    template<size_t N>
    struct log_convert<const char32_t[N]>
    {
        using type = const char32_t*;
    };

    template<class _Ty>
    using log_convert_t = typename log_convert<std::remove_reference_t<_Ty>>::type;
}
