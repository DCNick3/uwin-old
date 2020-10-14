//
// Created by dcnick3 on 10/14/20.
//

#pragma once

#include "uwin/kobj/kobj.h"
#include "uwin/kobj/ksurf_types.h"
#include "uwin/common_def.h" // for uw_rect_t

#include <memory>

#include <SDL.h>

// all pixel formats are RGB565

namespace uwin {
    class ksurf_sdl : public kobj {
    public:
        ksurf_sdl(int32_t width, int32_t height);

        void lock(uw_locked_surf_desc_t* locked);
        void unlock();

        static void blit(ksurf_sdl& src, const uw_rect_t* src_rect, ksurf_sdl& dst, uw_rect_t* dst_rect);
        static void blit_srckeyed(ksurf_sdl& src, const uw_rect_t* src_rect, ksurf_sdl& dst, uw_rect_t* dst_rect, uint16_t srckey_color);
        void fill(uw_rect_t* dst_rect, uint16_t color);

        static ksurf_sdl get_primary();

        inline SDL_Surface* get() const { return _surf.get(); }

        struct destroyer {
            void operator()(SDL_Surface* surf);
        };

    private:
        std::unique_ptr<SDL_Surface, destroyer> _surf;
    };
}