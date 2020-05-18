#pragma once
#include <exception>
#include "log_file.h"

namespace shark_log
{
    template<class _Ty>
    struct log_serialize
    {
        using read_type = _Ty;

        static bool write(const _Ty& value, log_file& file)
        {
            return file.write(&value, sizeof(_Ty));
        }
        static bool read(read_type& value, log_file& file)
        {
            return file.read(&value, sizeof(_Ty));
        }
    };

    template<class _Ty>
    using read_type_t = typename log_serialize<_Ty>::read_type;


    template<class _Ty>
    struct log_Cstring_serialize
    {
        using read_type = std::basic_string<_Ty>;

        static bool write(const _Ty* value, log_file& file)
        {
            const _Ty* end = value;
            while (*end) ++end;
            return file.write(value, sizeof(_Ty) * (end - value + 1));
        }
        static bool read(read_type& value, log_file& file)
        {
            _Ty ch;
            do
            {
                if (!file.read(&ch, sizeof(_Ty)))
                    return false;
                value += ch;
            } while (ch);
        }
    };

    template<class _Ty>
    struct log_serialize<std::basic_string<_Ty>>
    {
        using read_type = std::basic_string<_Ty>;

        static bool write(const std::basic_string<_Ty>& value, log_file& file)
        {
            return file.write(&value, sizeof(_Ty) * (value.size() + 1));
        }
        static bool read(read_type& value, log_file& file)
        {
            _Ty ch;
            do
            {
                if (!file.read(&ch, sizeof(_Ty)))
                    return false;
                value += ch;
            } while (ch);
        }
    };

    template<> struct log_serialize<const char*> : log_Cstring_serialize<char> {};
    template<> struct log_serialize<char*> : log_Cstring_serialize<char> {};
    template<> struct log_serialize<const wchar_t*> : log_Cstring_serialize<wchar_t> {};
    template<> struct log_serialize<wchar_t*> : log_Cstring_serialize<wchar_t> {};
    template<> struct log_serialize<const char32_t*> : log_Cstring_serialize<char32_t> {};
    template<> struct log_serialize<char32_t*> : log_Cstring_serialize<char32_t> {};
    template<> struct log_serialize<const char16_t*> : log_Cstring_serialize<char16_t> {};
    template<> struct log_serialize<char16_t*> : log_Cstring_serialize<char16_t> {};
#ifdef __cpp_char8_t
    template<> struct log_serialize<const char8_t*> : log_Cstring_serialize<char8_t> {};
    template<> struct log_serialize<char8_t*> : log_Cstring_serialize<char8_t> {};
#endif

    inline bool log_serialize_to_file(log_file& file)
    {
        return true;
    }
    template<class _Ty, class... _Rest>
    inline bool log_serialize_to_file(log_file& file, const _Ty& value, const _Rest&... rest)
    {
        if (!log_serialize<_Ty>::write(value, file))
            return false;
        return log_serialize_to_file(file, rest...);
    }
    template <class _Tuple, size_t... _Indices>
    inline bool log_serialize_tuple_to_file(log_file& file, _Tuple&& _Tpl, std::index_sequence<_Indices...>)
    {
        log_serialize_to_file(file, std::get<_Indices>(std::forward<_Tuple>(_Tpl))...);
        return true;
    }
    template<class... Args>
    inline bool log_serialize_to_file(log_file& file, const std::tuple<Args...>& value)
    {
        return log_serialize_tuple_to_file(file, value, std::index_sequence_for<Args...>{});
    }

    template<class _Ty>
    inline auto log_serialize_from_file(log_file& file) ->typename log_serialize<_Ty>::read_type
    {
        using read_type = typename log_serialize<_Ty>::read_type;
        read_type value;
        if (!log_serialize<_Ty>::read(value, file))
            throw std::logic_error("serialize failed");
        return value;
    }
}
