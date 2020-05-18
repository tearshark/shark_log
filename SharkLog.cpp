#include <iostream>
#include <chrono>

#include "log.h"

using namespace std::chrono;
using namespace std::literals;
using namespace shark_log;

extern void test_spsc_ring_queue();

//high_resolution_clock::now() : 45��ʱ������
//_log_tick() : 0.7��ʱ������
void test_tick_performace()
{
	uint64_t s = _log_tick();
	for (int i = 0; i < 10000; ++i)
		(void)high_resolution_clock::now();
	uint64_t e = _log_tick();
	uint64_t dt = e - s;
	fmt::print("now() cost time: {0} CPU cycles", dt);
}

int main()
{
	const uint64_t freq = _log_tick_freq();

    shark_log_initialize(shark_log_stdfile_factory(), "C:/logs/mylog-{0:04d}{1:02d}{2:02d}-{3:02d}{4:02d}{5:02d}.txt");

    std::string hello = "Hello World!"s;

    static_assert(std::is_same_v<decltype(shark_decval(3, 8.0f, "Hello World!")), log_info<int, float, const char *>>, "");

    sharkl_info("this is a test for disassembly {} / {}", 1, 2.0);

    std::atomic<uint64_t> dt{ 0 };

    const size_t K = 1;
    const size_t N = 100;
    for (int j = 0; j < K; ++j)
    {
        std::atomic<uint64_t> s;
        s.store(_log_tick());       //�������ñ���������

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

        dt.fetch_add(_log_tick() - s.load(std::memory_order_acquire));       //�������ñ���������

        std::this_thread::sleep_for(10ms);
    }

    shark_log_destroy();

    fmt::print("\nlog cost time: {0} ns", (dt.load() * 1000 / freq) / (K * N * 15));
}
