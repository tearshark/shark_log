#include <iostream>
#include <chrono>
#include <vector>
#include <algorithm>

#include "log.h"

using namespace std::chrono;
using namespace std::literals;
using namespace shark_log;

extern void test_spsc_ring_queue();

//high_resolution_clock::now() : 45个时钟周期
//_log_tick() : 0.7个时钟周期
void test_tick_performace()
{
	uint64_t s = _log_tick();
	for (int i = 0; i < 10000; ++i)
		(void)high_resolution_clock::now();
	uint64_t e = _log_tick();
	uint64_t dt = e - s;
	fmt::print("now() cost time: {0} CPU cycles", dt);
}

template<class _Callback>
void benchmark_log_callback(const uint64_t freq, const char * name, const _Callback& cb)
{
    const size_t K = 50;
    const size_t N = 2000;

    std::atomic<uint64_t> total{ 0 }, mintick{ (std::numeric_limits<uint64_t>::max)() }, stick;

    for (int j = 0; j < K; ++j)
    {
        for (int i = 0; i < N; ++i)
        {
            stick.store(__rdtsc());

            cb();

            uint64_t dt = __rdtsc() - stick.load();

            total += dt;
            mintick.store((std::min)(mintick.load(), dt));
        }

        shark_log_notify_format(nullptr);       //通知格式化线程有数据写入
        std::this_thread::sleep_for(10ms);      //给格式化线程足够的时间落地，以便于取得更好的写入延迟。
    }

    auto avg = (total * 1000 / freq) / (K * N);
    mintick = mintick * 1000 / freq;
    fmt::print("{0} cost time: min={1} ns, avg={2} ns.\r\n", name, mintick, avg);

    std::this_thread::sleep_for(200ms);
}

void benchmark_log()
{
    const uint64_t freq = _log_tick_freq();

    benchmark_log_callback(freq, "staticString", []
        {
        sharkl_info("Starting backup replica garbage collector thread"); 
        });

    benchmark_log_callback(freq, "staticString", []
        {
            sharkl_info("Starting backup replica garbage collector thread");
        });
    benchmark_log_callback(freq, "singleInteger", [] {
        sharkl_info("Backup storage speeds (min): {} MB/s read", 181);
        });
    benchmark_log_callback(freq, "twoIntegers", [] {
        sharkl_info("buffer has consumed {} bytes of extra storage, current allocation: {} bytes", 1032024, 1016544); 
        });
    benchmark_log_callback(freq, "singleDouble", [] {
        sharkl_info("Using tombstone ratio balancer with ratio = {}", 0.4); 
        });
    benchmark_log_callback(freq, "complexFormat", [] {
        sharkl_info("Initialized InfUdDriver buffers: {} receive buffers ({} MB), {} transmit buffers ({} MB), took {} ms", 50000, 97, 50, 0, 26.2f); 
        });
    benchmark_log_callback(freq, "stringConcat", [] {
        sharkl_info("Opened session with coordinator at basic+udp:host={}, port={}", "192.168.1.140", 12246);
        });
}

void test_log()
{
    std::atomic<uint64_t> dt{ 0 };

    const size_t K = 100;
    const size_t N = 200;

    for (int j = 0; j < K; ++j)
    {
        std::atomic<uint64_t> s;
        s.store(_log_tick());       //尽量不让编译器乱序

        for (int i = 0; i < N; ++i)
        {
            sharkl_info("this is a info");
            sharkl_info("this is a info with args {0}", 1.0);
            sharkl_info("this is a info with args {0}, {1}", 211, 5.0);
            sharkl_info("this is a info with args {0}, {1}, {2}, {3}", 3, 8.0f, "foo", 10.0);
            sharkl_info("this is a info with args {0}, {1}, {2}, {3}, {4}", 54321, 3, 8.0f, "bar", 10.0);

            sharkl_warn("this is a warning");
            sharkl_warn("this is a warning with args {0}", 1.0);
            sharkl_warn("this is a warning with args {0}, {1}", 211, 5.0);
            sharkl_warn("this is a warning with args {0}, {1}, {2}, {3}", 3, 8.0f, "foo", 10.0);
            sharkl_warn("this is a warning with args {0}, {1}, {2}, {3}, {4}", 54321, 3, 8.0f, "bar", 10.0);

            sharkl_eror("this is a error");
            sharkl_eror("this is a error with args {0}", 1.0);
            sharkl_eror("this is a error with args {0}, {1}", 211, 5.0);
            sharkl_eror("this is a error with args {0}, {1}, {2}, {3}", 3, 8.0f, "foo", 10.0);
            sharkl_eror("this is a error with args {0}, {1}, {2}, {3}, {4}", 54321, 3, 8.0f, "bar", 10.0);
        }

        uint64_t t = _log_tick() - s.load(std::memory_order_acquire);
        dt.fetch_add(t);       //尽量不让编译器乱序

        shark_log_notify_format(nullptr);       //通知格式化线程有数据写入
        std::this_thread::sleep_for(10ms);      //给格式化线程足够的时间落地，以便于取得更好的写入延迟。
    }

    const uint64_t freq = _log_tick_freq();
    auto avg = (dt.load() * 1000 / freq) / (K * N * 15);
    fmt::print("\nlog cost avg time: {0} ns.", avg);
}

int main()
{
	static_assert(std::is_same_v<decltype(shark_decval(3, 8.0f, "Hello World!")), log_info<int, float, const char*>>, "");

    shark_log_initialize(shark_log_stdfile_factory(), true, "C:/logs/mylog-{0:04d}{1:02d}{2:02d}-{3:02d}{4:02d}{5:02d}.{6}");
    sharkl_info("this is a test for disassembly {} / {}", 1, 2.0);

    benchmark_log();
	shark_log_destroy();
}
