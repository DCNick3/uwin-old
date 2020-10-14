//
// Created by dcnick3 on 10/14/20.
//

#include "uwin/sdl/kobj/ksurf_sdl.h"
#include "uwin/util/align.h"
#include "uwin/mem.h"
#include "uwin/uwin.h"

#include <mutex>
#include <condition_variable>

#include <cassert>

namespace uwin {

    static uint32_t sdl_surf_request_event = -1; // TODO: !!! DUMMY VALUE !!!
    static pthread_t main_thread;  // TODO: !!! DUMMY VALUE !!!

    static bool interactive_mode = true;

    static SDL_Rect uw_rect_to_sdl_rect(const uw_rect_t* srect) {
        SDL_Rect rect;
        rect.x = srect->left;
        rect.y = srect->top;
        rect.w = srect->right - srect->left + 1;
        rect.h = srect->bottom - srect->top + 1;
        return rect;
    }

    static void flush_surf(SDL_Surface* surf) {
        char filename[80];
        //sprintf(filename, "sdl_dump/%p_%08d.bpm", surf, bmp_id++);
        //SDL_SaveBMP(primary_surface, filename);

        if (interactive_mode) {
            // TODO: DUMMY IMPLEMENTATION
            /*if (surf == (uw_surf_t *) primary_surface) {
                SDL_UpdateWindowSurface(window);
            }*/
        }
    }

    // SDL requires that all graphics operations are executed in the main thread. So, here is some ugly way to do it
    struct surf_request {
        bool done = false;
        std::mutex m;
        std::condition_variable cv;

        void execute_remote() {
            SDL_Event event;
            SDL_zero(event);
            event.type = sdl_surf_request_event;
            event.user.code = 0;
            event.user.data1 = (void*)this;
            event.user.data2 = nullptr;
            int r = SDL_PushEvent(&event);
            assert(r >= 0);

            {
                std::unique_lock<std::mutex> lk(m);
                cv.wait(lk, [this] { return done; });
            }
        }

        virtual void actually_execute() {
            done = true;
            cv.notify_all();
        }
    };

    struct alloc_surf_request : public surf_request {
        void actually_execute() override {
            // we want to manually allocate surface for it's data to be accessible by guest
            size_t pitch = uwin::align_up(width, 4) * 2;
            size_t sz = uwin::align_up(pitch * height, uw_host_page_size);
            uint32_t v = uw_target_map_memory_dynamic(sz, UW_PROT_RW);
            assert(v != (uint32_t) -1);
            void *mem = g2h(v);

            res = SDL_CreateRGBSurfaceWithFormatFrom(mem, width, height, 16, pitch, SDL_PIXELFORMAT_RGB565);
            assert(res != NULL);
            SDL_SetSurfaceRLE(res, 0);

            surf_request::actually_execute();
        }

        static SDL_Surface* execute_static(int32_t width, int32_t height) {
            assert(!pthread_equal(pthread_self(), main_thread));
            alloc_surf_request rq;
            rq.width = width;
            rq.height = height;
            rq.execute_remote();
            return rq.res;
        }

        int32_t width;
        int32_t height;
        SDL_Surface *res;
    };

    struct free_surf_request : public surf_request {
        void actually_execute() override {
            void *pixdata = surf->pixels;
            size_t pitch = uwin::align_up(surf->w, 4);
            size_t sz = uwin::align_up(pitch * surf->h * 2, uw_host_page_size);
            uw_unmap_memory(h2g(pixdata), sz);
            SDL_FreeSurface(surf);

            surf_request::actually_execute();
        }


        static void execute_static(SDL_Surface* surf) {
            // TODO: DUMMY IMPLEMENTATION
            //if (surf == (uw_surf_t *) primary_surface)
            //    return;

            assert(!pthread_equal(pthread_self(), main_thread));

            free_surf_request rq;
            rq.surf = surf;
            rq.execute_remote();
        }

        SDL_Surface *surf;
    };

    struct lock_surf_request : public surf_request {
        void actually_execute() override {
            //assert(&surf->sdl_surf != primary_surface); // the pointer to data would be not in the guest address space
            int r = SDL_LockSurface(surf);
            assert(r == 0);
            locked->data  = surf->pixels;
            locked->w     = surf->w;
            locked->h     = surf->h;
            locked->pitch = surf->pitch;

            surf_request::actually_execute();
        }

        static void static_execute(SDL_Surface* surf, uw_locked_surf_desc_t* locked) {
            assert(!pthread_equal(pthread_self(), main_thread));
            lock_surf_request rq;
            rq.surf = surf;
            rq.locked = locked;
            rq.execute_remote();
            return;
        }

        SDL_Surface *surf;
        uw_locked_surf_desc_t *locked;
    };

    struct unlock_surf_request : public surf_request {
        void actually_execute() override {

            //  assert(&surf->sdl_surf != primary_surface); // the pointer to data would be not in the guest address space
            SDL_UnlockSurface(surf);

            flush_surf(surf);
            surf_request::actually_execute();
        }

        static void static_execute(SDL_Surface* surf) {
            if (!pthread_equal(pthread_self(), main_thread)) {
                unlock_surf_request rq;
                rq.surf = surf;
                rq.execute_remote();
                return;
            }
        }

        SDL_Surface *surf;
    };

    struct blit_surf_request : public surf_request {
        void actually_execute() override {

            int r = 0;

            r = SDL_SetColorKey(src, SDL_FALSE, 0);
            assert(!r);

            // TODO: clipping of the dst rectangle is not done
            SDL_Rect srect, drect;
            SDL_Rect *psrect = NULL, *pdrect = NULL;
            if (src_rect) {
                srect = uw_rect_to_sdl_rect(src_rect);
                psrect = &srect;
            }
            if (dst_rect) {
                drect = uw_rect_to_sdl_rect(dst_rect);
                pdrect = &drect;
            }

            r = SDL_BlitSurface(src, psrect, dst, pdrect);
            assert(!r);

            flush_surf(dst);
            surf_request::actually_execute();
        }

        static void execute_static(SDL_Surface* src, const uw_rect_t* src_rect, SDL_Surface* dst, uw_rect_t* dst_rect) {
            assert(!pthread_equal(pthread_self(), main_thread));
            blit_surf_request rq;
            rq.src = src;
            rq.src_rect = src_rect;
            rq.dst = dst;
            rq.dst_rect = dst_rect;
            rq.execute_remote();
            return;
        }

        SDL_Surface* src;
        const uw_rect_t* src_rect;
        SDL_Surface* dst;
        uw_rect_t* dst_rect;
    };

    struct blit_srckeyed_surf_request : public surf_request {
        void actually_execute() override {

            int r = 0;

            r = SDL_SetColorKey(src, SDL_TRUE, srckey_color);
            assert(!r);

            // TODO: clipping of the dst rectangle is not done
            SDL_Rect srect, drect;
            SDL_Rect *psrect = NULL, *pdrect = NULL;
            if (src_rect) {
                srect = uw_rect_to_sdl_rect(src_rect);
                psrect = &srect;
            }
            if (dst_rect) {
                drect = uw_rect_to_sdl_rect(dst_rect);
                pdrect = &drect;
            }

            r = SDL_BlitSurface(src, psrect, dst, pdrect);
            assert(!r);

            flush_surf(dst);
            surf_request::actually_execute();
        }

        static void execute_static(SDL_Surface* src, const uw_rect_t* src_rect, SDL_Surface* dst, uw_rect_t* dst_rect, uint16_t srckey_color) {
            if (!pthread_equal(pthread_self(), main_thread)) {
                blit_srckeyed_surf_request rq;
                rq.src = src;
                rq.src_rect = src_rect;
                rq.dst = dst;
                rq.dst_rect = dst_rect;
                rq.srckey_color = srckey_color;
                rq.execute_remote();
                return;
            }
        }

        SDL_Surface* src;
        const uw_rect_t* src_rect;
        SDL_Surface* dst;
        uw_rect_t* dst_rect;
        uint16_t srckey_color;
    };

    struct fill_surf_request : public surf_request {
        void actually_execute() override {

            if (dst_rect) {
                // TODO: clipping of the dst rectangle is not done
                SDL_Rect rect = uw_rect_to_sdl_rect(dst_rect);
                int r = SDL_FillRect(dst, &rect, color);
                assert(!r);
            } else {
                int r = SDL_FillRect(dst, NULL, color);
                assert(!r);
            }

            flush_surf(dst);
            surf_request::actually_execute();
        }

        static void execute_static(SDL_Surface* dst, uw_rect_t* dst_rect, uint16_t color) {
            if (interactive_mode) {
                if (!pthread_equal(pthread_self(), main_thread)) {
                    fill_surf_request rq;
                    rq.dst = dst;
                    rq.dst_rect = dst_rect;
                    rq.color = color;
                    rq.execute_remote();
                    return;
                }
            }
        }

        SDL_Surface* dst;
        uw_rect_t* dst_rect;
        uint16_t color;
    };


    ksurf_sdl::ksurf_sdl(int32_t width, int32_t height) :
        _surf(alloc_surf_request::execute_static(width, height)) {}

    void ksurf_sdl::lock(uwin::uw_locked_surf_desc_t *locked) {
        lock_surf_request::static_execute(get(), locked);
    }

    void ksurf_sdl::unlock() {
        unlock_surf_request::static_execute(get());
    }

    void ksurf_sdl::blit(ksurf_sdl &src, const uw_rect_t *src_rect, ksurf_sdl &dst, uw_rect_t *dst_rect) {
        blit_surf_request::execute_static(src.get(), src_rect, dst.get(), dst_rect);
    }

    void ksurf_sdl::blit_srckeyed(ksurf_sdl &src, const uw_rect_t *src_rect, ksurf_sdl &dst, uw_rect_t *dst_rect,
                                  uint16_t srckey_color) {
        blit_srckeyed_surf_request::execute_static(src.get(), src_rect, dst.get(), dst_rect, srckey_color);
    }

    void ksurf_sdl::fill(uw_rect_t *dst_rect, uint16_t color) {
        fill_surf_request::execute_static(get(), dst_rect, color);
    }

    ksurf_sdl ksurf_sdl::get_primary() {
        std::terminate();
    }

    void ksurf_sdl::destroyer::operator()(SDL_Surface *surf) {
        // should be quite wary with this; might fail spectacularly (probably with deadlock) if the processing thead is inactive
        free_surf_request::execute_static(surf);
    }
}
