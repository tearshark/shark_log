#include <iostream>
#include <chrono>

#include "log.h"

using namespace std::chrono;
using namespace std::literals;
using namespace shark_log;

int main()
{
    shark_log_initialize(shark_log_stdfile_factory(), "C:/logs/mylog-{0:04d}{1:02d}{2:02d}-{3:02d}{4:02d}{5:02d}.bin");

    std::string hello = "Hello World!"s;

    static_assert(std::is_same_v<decltype(shark_decval(3, 8.0f, "Hello World!")), log_info<int, float, const char *>>, "");

    sharkl_info("this is a test for disassembly {} / {}", 1, 2.0);

    std::atomic<uint64_t> dt{ 0 };

    const size_t K = 1;
    const size_t N = 1000;
    for (int j = 0; j < K; ++j)
    {
        std::atomic<uint64_t> s;
        s.store(_log_tick());       //æ°¡ø≤ª»√±‡“Î∆˜¬“–Ú

        for (int i = 0; i < N; ++i)
        {
            sharkl_info("this is a info");
            sharkl_info("this is a info with args {0}", 1.0);
            sharkl_info("this is a info with args {0}, {1}", 2ll, 5.0);
            sharkl_info("this is a info with args {0}, {1}, {2}, {3}", 3, 8.0f, "foo", 10.0);
            sharkl_info("this is a info with args {0}, {1}, {2}, {3}, {4}", 2ll, 3, 8.0f, "bar", 10.0);

            sharkl_warn("this is a warning");
            sharkl_warn("this is a warning with args {0}", 1.0);
            sharkl_warn("this is a warning with args {0}, {1}", 2ll, 5.0);
            sharkl_warn("this is a warning with args {0}, {1}, {2}, {3}", 3, 8.0f, "foo", 10.0);
            sharkl_warn("this is a warning with args {0}, {1}, {2}, {3}, {4}", 2ll, 3, 8.0f, "bar", 10.0);

            sharkl_eror("this is a error");
            sharkl_eror("this is a error with args {0}", 1.0);
            sharkl_eror("this is a error with args {0}, {1}", 2ll, 5.0);
            sharkl_eror("this is a error with args {0}, {1}, {2}, {3}", 3, 8.0f, "foo", 10.0);
            sharkl_eror("this is a error with args {0}, {1}, {2}, {3}, {4}", 2ll, 3, 8.0f, "bar", 10.0);
        }

        dt.fetch_add(_log_tick() - s.load(std::memory_order_acquire));       //æ°¡ø≤ª»√±‡“Î∆˜¬“–Ú

        std::this_thread::sleep_for(10ms);
    }

    shark_log_destroy();

    fmt::print("\nlog cost time: {0} CPU cycles", dt.load() / (K * N * 15));
}
