#include "barrier.hxx"


namespace utility
{
    void barrier::wait()
    {
        std::unique_lock<std::mutex> lock{mutex};
        auto lgen = generation;

        if (--count == 0) {
            ++generation;
            count = threshold;
            cv.notify_all();
        }

        else cv.wait(lock, [this, lgen] { return lgen != generation || drop_it; });
    }

    void barrier::drop()
    {
        std::lock_guard<std::mutex> lock{mutex};
        drop_it = true;
        cv.notify_all();
    }
}
