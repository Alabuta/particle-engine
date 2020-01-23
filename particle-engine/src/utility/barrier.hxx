#pragma once

#include <atomic>
#include <thread>
#include <condition_variable>


namespace utility
{
    class barrier {
    public:
        barrier(std::size_t count) : count{count}, threshold{count}, generation{0}, drop_it{false} { }

        void wait();

        void drop();

    private:
        std::mutex mutex;
        std::condition_variable cv;

        std::size_t count;
        std::size_t threshold;
        std::size_t generation;

        std::atomic_bool drop_it;
    };
}