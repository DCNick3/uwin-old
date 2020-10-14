//
// Created by dcnick3 on 10/14/20.
//

#pragma once

#include "kobj.h"

#include <mutex>
#include <chrono>

namespace uwin {
    class kmut : public kobj {
    public:
        inline uint32_t lock(std::uint64_t timeout_us) {
            // TODO: overflow?
            if (_mutex.try_lock_for(std::chrono::microseconds(timeout_us)))
                return 0;
            return -1;
        }

        inline void unlock() {
            _mutex.unlock();
        }

        inline std::timed_mutex& mutex() {
            return _mutex;
        }
    private:
        std::timed_mutex _mutex;
    };
}