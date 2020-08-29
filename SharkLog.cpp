#include <iostream>
#include <fstream>
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

void benchmark_parse_result(const char* name, const char* path)
{
    std::ifstream ifs(path, std::ios::in);
    if (!ifs.is_open())
    {
        fmt::print("open file '{}' failed.", path);
        return;
    }

    std::vector<uint64_t> ticks;
    ticks.reserve(20000);

    std::string str;
    while (!ifs.eof())
    {
        std::getline(ifs, str);
        if (str.empty())
            break;

        uint64_t t = std::strtoull(str.c_str(), nullptr, 10);
        std::getline(ifs, str);

        ticks.push_back(t);
    }

    if (ticks.size() > 100)
    {
        std::sort(ticks.begin(), ticks.end());

        uint64_t mintick = (std::numeric_limits<uint64_t>::max)(), total = 0;

        uint64_t start = ticks.front();
        ticks.front() = 0;

        for(auto iter = ticks.begin() + 1; iter != ticks.end(); ++iter)
        {
            auto& t = *iter;
            auto tmp = t;
            t -= start;
            start = tmp;

            mintick = (std::min)(mintick, t);
            total += t;
        }
        std::sort(ticks.begin(), ticks.end());

        const uint64_t freq = _log_tick_freq();

        uint64_t avg = total / (ticks.size() - 1);
        uint64_t th50 = ticks[ticks.size() / 2];
        uint64_t th999 = ticks[ticks.size() * 999 / 1000];
		uint64_t last = ticks.back();

        fmt::print("'{}' cost time: min={} ns, avg={} ns, 50.0th={} ns, 99.9th={} ns, last={} ns\r\n", 
            name, 
            mintick * 1000 / freq, 
            avg * 1000 / freq, 
            th50 * 1000 / freq, 
            th999 * 1000 / freq,
            last * 1000/ freq);
    }
}
template<class _Callback>
void benchmark_log_callback(const char* name, const _Callback& cb)
{
    const char* path = "C:/logs/mylog-bench.txt";
    ::DeleteFileA(path);
    shark_log_initialize(shark_log_stdfile_factory(), false, path);

    const size_t TN = 11;
    std::thread log_thread[TN];
    for (size_t i = 0; i < TN; ++i)
    {
        log_thread[i] = std::thread([cb]
        {
            SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL);
            SetThreadAffinityMask(GetCurrentThread(), 2);
                
            const size_t N = 50000;
            for (int i = 0; i < N; ++i)
            {
                cb();
            }

            shark_log_notify_format(nullptr);
        });
    }

    for (size_t i = 0; i < TN; ++i)
        log_thread[i].join();

    std::this_thread::sleep_for(1000ms);
    shark_log_destroy();

    benchmark_parse_result(name, path);

    std::this_thread::sleep_for(100ms);
}

void benchmark_log()
{
    benchmark_log_callback("staticString", []
        {
        sharkl_info("Starting backup replica garbage collector thread");
        });
    benchmark_log_callback("stringConcat", [] {
        sharkl_info("Opened session with coordinator at {}", "basic+udp:host=192.168.1.140,port=12246");
        });
    benchmark_log_callback("singleInteger", [] {
        sharkl_info("Backup storage speeds (min): {} MB/s read", 181);
        });
    benchmark_log_callback("twoIntegers", [] {
        sharkl_info("buffer has consumed {} bytes of extra storage, current allocation: {} bytes", 1032024, 1016544);
        });
    benchmark_log_callback("singleDouble", [] {
        sharkl_info("Using tombstone ratio balancer with ratio = {}", 0.4);
        });
    benchmark_log_callback("complexFormat", [] {
        sharkl_info("Initialized InfUdDriver buffers: {} receive buffers ({} MB), {} transmit buffers ({} MB), took {} ms", 50000, 97, 50, 0, 26.2f);
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

int main(int argc, const char* argv[])
{
	static_assert(std::is_same_v<decltype(shark_decval(3, 8.0f, "Hello World!")), log_info<int, float, const char*>>, "");

    benchmark_log();

    //shark_log_initialize(shark_log_stdfile_factory(), false, "C:/logs/mylog-bench.txt");
    //sharkl_info("this is a test for disassembly {} / {}", 1, 2.0);

	//shark_log_destroy();
}
