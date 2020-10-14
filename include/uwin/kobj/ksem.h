//
// Created by dcnick3 on 10/14/20.
//

#pragma once

#include "kobj.h"

#include <mutex>
#include <condition_variable>

namespace uwin {
    class ksem : public kobj {
        // C++ has no native semaphore...
        // Oh, well
    public:
        ksem(int count_ = 0) : _count(count_) {}

        inline void post()
        {
            std::unique_lock<std::mutex> lock(_mutex);
            _count++;
            _cv.notify_one();
        }

        inline void wait(std::uint64_t timeout_us)
        {
            if (timeout_us != 0xffffffffffffffffULL)
                // not yet implemented
                std::terminate();

            std::unique_lock<std::mutex> lock(_mutex);


            while(_count == 0){
                _cv.wait(lock);
            }
            _count--;
        }

    private:
        std::mutex _mutex;
        std::condition_variable _cv;
        int _count;
    };
}