//
// Created by dcnick3 on 10/14/20.
//

#pragma once

#include "kobj.h"
#include "kmut.h"

#include <condition_variable>
#include <chrono>

namespace uwin {
    class kcond : public kobj {
    public:

        inline void signal() {
            _cv.notify_one();
        }

        inline void broadcast() {
            _cv.notify_all();
        }

        inline uint32_t wait(kmut& mut, std::uint64_t timeout_us) {
            // TODO: overflow?
            if (_cv.wait_for(mut.mutex(), std::chrono::microseconds(timeout_us)) == std::cv_status::timeout)
                return -1;
            return 0;
        }

    private:
        std::condition_variable_any _cv;
    };
}