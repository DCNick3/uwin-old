//
// Created by dcnick3 on 10/14/20.
//

#pragma once

#include <cstdint>

namespace uwin {
    typedef struct uw_locked_surf_desc {
        void *data;
        uint32_t pitch;
        uint32_t w, h;
    } uw_locked_surf_desc_t;

    typedef struct target_uw_locked_surf_desc {
        uint32_t data;
        uint32_t pitch;
        uint32_t w, h;
    } __attribute__((packed)) target_uw_locked_surf_desc_t;
}
