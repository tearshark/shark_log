#pragma once
#include <stdint.h>
#include <string>
/*
#if __has_include(<concepts>)
#include <concepts>
#define SHARK_LOG_ENABLE_CONCEPT 0
#else
*/
#define SHARK_LOG_ENABLE_CONCEPT 0
//#endif

namespace shark_log
{
#if SHARK_LOG_ENABLE_CONCEPT
    template<typename _Ty>
    concept _LogFile = requires(_Ty && v, const void* w, void* r, size_t l)
    {
        { v.write(w, l) } ->std::same_as<bool>;
        { v.read(r, l) } ->std::same_as<bool>;
    };
#else
#define _LogFile typename
#endif

    struct log_file
    {
        virtual ~log_file() = default;
        virtual bool write(const void*, size_t) = 0;
        virtual bool read(void*, size_t) = 0;
    };

    struct log_file_factory
    {
        virtual log_file* create(const std::string& path, bool writeMode) = 0;
    };

    log_file_factory* shark_log_stdfile_factory();
}
