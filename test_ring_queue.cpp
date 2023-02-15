#include <iostream>
#include <chrono>
#include <thread>

#include "spsc_ring_queue.h"
#include "log_info.h"

using namespace std::chrono;
using namespace std::literals;
using namespace shark_log;

void test_spsc_ring_queue()
{
    std::atomic<int64_t> t{ 0 };
    spsc_ring_queue<int> q(4096);

    for (int i = 0; i < 2000; ++i)
    {
        int val = rand();
        t.fetch_add(val);

        while (!q.try_push(val));
    }

    for (int i = 0; i < 2000; ++i)
    {
        int val;
        q.try_pop(val);
        t.fetch_sub(val);
    }

    fmt::print("result is {0}. \r\n", t.load());

    t.store(0);

    std::thread push([&]()
        {
            for (int i = 0; i < 10000; ++i)
            {
                int val = rand();
                t.fetch_add(val);

                while (!q.try_push(val));
            }
        });

    std::thread pop([&]()
        {
            for (int i = 0; i < 10000; )
            {
                i += (int)q.consume_all([&t](int val)
                    {
                        t.fetch_sub(val);
                    });
            }
        });

    push.join();
    pop.join();

    fmt::print("result is {0}.\r\n", t.load());
}
