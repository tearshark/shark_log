#pragma once
#include <stdint.h>

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

namespace shark_log
{
    const size_t _LOG_ALIGN_LSHIFT = (sizeof(void*) == 4) ? 5 : 6;
    const size_t _LOG_ALIGN_REQ = (size_t)1u << _LOG_ALIGN_LSHIFT;
    const size_t _LOG_BUFFER_CNT = 4;
    const size_t _LOG_MAX_INFO = _LOG_ALIGN_REQ * (size_t)(1u << _LOG_BUFFER_CNT);
    const size_t _LOG_CACHE_SIZE = 1024 * 1024;

    template<class _Ty>
#if _HAS_CXX17 || _HAS_CXX20 || __cplusplus >= 201703L
    constexpr
#else
    inline
#endif
    size_t _log_align_idx() noexcept
    {
        size_t idx = 0;
        for (; sizeof(_Ty) > ((size_t)_LOG_ALIGN_REQ << idx); ++idx);
        return idx;
    }

    inline uint64_t _log_tick() noexcept
    {
        return __rdtsc();
    }

    //Ã¿Î¢ÃëµÄtick
    extern uint64_t _log_tick_freq() noexcept;

    enum struct log_level : uint8_t
    {
        info,
        warn,
        eror,
    };

#if _HAS_CXX17 || _HAS_CXX20 || __cplusplus >= 201703L
	constexpr
#else
	inline
#endif
	const char* _log_level_string(const log_level lvl) noexcept
    {
        switch (lvl)
        {
        case log_level::info: return "info";
        case log_level::warn: return "warn";
        case log_level::eror: return "eror";
        }
        return "unkn";
    }
}
