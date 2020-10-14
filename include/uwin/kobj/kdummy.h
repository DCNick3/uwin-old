//
// Created by dcnick3 on 10/14/20.
//

#pragma once

#include "kobj.h"

#include <cstdint>

namespace uwin {
    class kdummy : public kobj {
        std::uint32_t type, data;
    public:
        std::uint32_t get_type() const { return type; }
        std::uint32_t get_data() const { return data; }

        kdummy(std::uint32_t type, std::uint32_t data) : type(type), data(data)
        {}
    };
}