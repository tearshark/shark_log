#pragma once
#include <stdint.h>
#include <chrono>

#ifdef _MSC_VER
#include <intrin.h>
#else
#include <x86intrin.h>
#endif

namespace shark_log
{
    //Ã¿Î¢ÃëµÄtick
	uint64_t _log_tick_freq()
    {
        uint64_t stick = __rdtsc();
        auto sclock = std::chrono::high_resolution_clock::now();

        for (;;)
        {
            auto dt = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now() - sclock);
            if (dt.count() >= 1000)
                break;
        }

		uint64_t etick = __rdtsc();
		auto eclock = std::chrono::high_resolution_clock::now();

		auto dt = std::chrono::duration_cast<std::chrono::microseconds>(eclock - sclock);

        return (etick - stick) / dt.count();
	}
}
