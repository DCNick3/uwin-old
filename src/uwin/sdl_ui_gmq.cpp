
#include "uwin/uwin.h"
#include "uwin/util/align.h"

#include <assert.h>

#include <SDL.h>
#include <semaphore.h>
#include <mutex>
#include <condition_variable>

// provides gfx, snd & input libraries

struct uw_surf {
    SDL_Surface sdl_surf;
};

static uint32_t sdl_gmq_event, sdl_surf_request_event;
static bool interactive_mode = false;

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
        res = uw_ui_surf_alloc(width, height);
        surf_request::actually_execute();
    }

    int32_t width;
    int32_t height;
    uw_surf_t *res;
};

struct free_surf_request : public surf_request {
    void actually_execute() override {
        uw_ui_surf_free(surf);
        surf_request::actually_execute();
    }

    uw_surf_t *surf;
};

struct lock_surf_request : public surf_request {
    void actually_execute() override {
        uw_ui_surf_lock(surf, locked);
        surf_request::actually_execute();
    }

    uw_surf_t *surf;
    uw_locked_surf_desc_t *locked;
};

struct unlock_surf_request : public surf_request {
    void actually_execute() override {
        uw_ui_surf_unlock(surf);
        surf_request::actually_execute();
    }

    uw_surf_t *surf;
};

struct blit_surf_request : public surf_request {
    void actually_execute() override {
        uw_ui_surf_blit(src, src_rect, dst, dst_rect);
        surf_request::actually_execute();
    }

    uw_surf_t* src;
    const uw_rect_t* src_rect;
    uw_surf_t* dst;
    uw_rect_t* dst_rect;
};

struct blit_srckeyed_surf_request : public surf_request {
    void actually_execute() override {
        uw_ui_surf_blit_srckeyed(src, src_rect, dst, dst_rect, srckey_color);
        surf_request::actually_execute();
    }

    uw_surf_t* src;
    const uw_rect_t* src_rect;
    uw_surf_t* dst;
    uw_rect_t* dst_rect;
    uint16_t srckey_color;
};

struct fill_surf_request : public surf_request {
    void actually_execute() override {
        uw_ui_surf_fill(dst, dst_rect, color);
        surf_request::actually_execute();
    }

    uw_surf_t* dst;
    uw_rect_t* dst_rect;
    uint16_t color;
};

static SDL_Window* window;
static SDL_Surface* primary_surface = nullptr;

static pthread_t main_thread;

void uw_ui_initialize(int interactive) {
    main_thread = pthread_self();

    interactive_mode = interactive != 0;

    SDL_SetHint(SDL_HINT_VIDEO_X11_NET_WM_BYPASS_COMPOSITOR, "1");

    if (interactive_mode) {
        if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) != 0) {
            uw_log("Unable to initialize SDL: %s\n", SDL_GetError());
            abort();
        }
        window = SDL_CreateWindow("uwin-sdl",
                                  SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                  800, 600,
                                  SDL_WINDOW_MINIMIZED); // this one is temporary, for debugging
        if (window == NULL) {
            uw_log("Could not create window: %s\n", SDL_GetError());
            abort();
        }

        primary_surface = SDL_GetWindowSurface(window);
        SDL_ShowCursor(SDL_DISABLE);

        SDL_FillRect(primary_surface, NULL, 0);

        SDL_UpdateWindowSurface(window);
    } else {
        if (SDL_Init(SDL_INIT_AUDIO) != 0) {
            uw_log("Unable to initialize SDL: %s\n", SDL_GetError());
            abort();
        }

        size_t sz = UW_ALIGN_UP(800 * 600, uw_host_page_size);
        uint32_t v = uw_target_map_memory_dynamic(sz, UW_PROT_RW);
        assert(v != (uint32_t) -1);
        void *mem = g2h(v);

        primary_surface = SDL_CreateRGBSurfaceWithFormatFrom(mem, 800, 600, 16, 800, SDL_PIXELFORMAT_RGB565);
        assert(primary_surface != nullptr);
    }
}

void uw_ui_finalize(void) {
    if (interactive_mode) {
        SDL_Window *w = window;
        if (w != NULL)
            SDL_DestroyWindow(w);
        SDL_Quit();
    }
}



/*
 * gmq stuff
 */

void uw_gmq_initialize(void) {
    sdl_gmq_event = SDL_RegisterEvents(2);

    assert(sdl_gmq_event != -1);

    sdl_surf_request_event = sdl_gmq_event + 1;
}

void uw_gmq_finalize(void) {
}

void uw_gmq_post(int32_t message, void* param1, void* param2) {
    SDL_Event event;
    SDL_zero(event);
    event.type = sdl_gmq_event;
    event.user.code = message;
    event.user.data1 = param1;
    event.user.data2 = param2;
    int r = SDL_PushEvent(&event);
    assert(r >= 0);
}

static uint32_t sdl_scancode_to_gmq_key(uint32_t scancode) {
    switch (scancode) {
        case SDL_SCANCODE_UNKNOWN:      goto unsupp;
        case SDL_SCANCODE_A:            return 'A';
        case SDL_SCANCODE_B:            return 'B';
        case SDL_SCANCODE_C:            return 'C';
        case SDL_SCANCODE_D:            return 'D';
        case SDL_SCANCODE_E:            return 'E';
        case SDL_SCANCODE_F:            return 'F';
        case SDL_SCANCODE_G:            return 'G';
        case SDL_SCANCODE_H:            return 'H';
        case SDL_SCANCODE_I:            return 'I';
        case SDL_SCANCODE_J:            return 'J';
        case SDL_SCANCODE_K:            return 'K';
        case SDL_SCANCODE_L:            return 'L';
        case SDL_SCANCODE_M:            return 'M';
        case SDL_SCANCODE_N:            return 'N';
        case SDL_SCANCODE_O:            return 'O';
        case SDL_SCANCODE_P:            return 'P';
        case SDL_SCANCODE_Q:            return 'Q';
        case SDL_SCANCODE_R:            return 'R';
        case SDL_SCANCODE_S:            return 'S';
        case SDL_SCANCODE_T:            return 'T';
        case SDL_SCANCODE_U:            return 'U';
        case SDL_SCANCODE_V:            return 'V';
        case SDL_SCANCODE_W:            return 'W';
        case SDL_SCANCODE_X:            return 'X';
        case SDL_SCANCODE_Y:            return 'Y';
        case SDL_SCANCODE_Z:            return 'Z';
        case SDL_SCANCODE_1:            return '1';
        case SDL_SCANCODE_2:            return '2';
        case SDL_SCANCODE_3:            return '3';
        case SDL_SCANCODE_4:            return '4';
        case SDL_SCANCODE_5:            return '5';
        case SDL_SCANCODE_6:            return '6';
        case SDL_SCANCODE_7:            return '7';
        case SDL_SCANCODE_8:            return '8';
        case SDL_SCANCODE_9:            return '9';
        case SDL_SCANCODE_0:            return '0';
        case SDL_SCANCODE_RETURN:       return VK_RETURN;
        case SDL_SCANCODE_ESCAPE:       return VK_ESCAPE;
        case SDL_SCANCODE_BACKSPACE:    return VK_BACK;
        case SDL_SCANCODE_TAB:          return VK_TAB;
        case SDL_SCANCODE_SPACE:        return VK_SPACE;
        case SDL_SCANCODE_MINUS:        return VK_OEM_MINUS;
        case SDL_SCANCODE_EQUALS:       return VK_OEM_PLUS;
        case SDL_SCANCODE_LEFTBRACKET:  return VK_OEM_4;
        case SDL_SCANCODE_RIGHTBRACKET: return VK_OEM_6;
        case SDL_SCANCODE_BACKSLASH:    return VK_OEM_5;
        case SDL_SCANCODE_SEMICOLON:    return VK_OEM_1;
        case SDL_SCANCODE_APOSTROPHE:   return VK_OEM_7;
        case SDL_SCANCODE_GRAVE:        return VK_OEM_3;
        case SDL_SCANCODE_COMMA:        return VK_OEM_COMMA;
        case SDL_SCANCODE_PERIOD:       return VK_OEM_PERIOD;
        case SDL_SCANCODE_SLASH:        return VK_OEM_2;
        case SDL_SCANCODE_F1:           return VK_F1;
        case SDL_SCANCODE_F2:           return VK_F2;
        case SDL_SCANCODE_F3:           return VK_F3;
        case SDL_SCANCODE_F4:           return VK_F4;
        case SDL_SCANCODE_F5:           return VK_F5;
        case SDL_SCANCODE_F6:           return VK_F6;
        case SDL_SCANCODE_F7:           return VK_F7;
        case SDL_SCANCODE_F8:           return VK_F8;
        case SDL_SCANCODE_F9:           return VK_F9;
        case SDL_SCANCODE_F10:          return VK_F10;
        case SDL_SCANCODE_F11:          return VK_F11;
        case SDL_SCANCODE_F12:          return VK_F12;
        case SDL_SCANCODE_PRINTSCREEN:  return VK_SNAPSHOT;
        case SDL_SCANCODE_SCROLLLOCK:   return VK_SCROLL;
        case SDL_SCANCODE_PAUSE:        return VK_PAUSE;
        case SDL_SCANCODE_INSERT:       return VK_INSERT;
        case SDL_SCANCODE_HOME:         return VK_HOME;
        case SDL_SCANCODE_PAGEUP:       return VK_PRIOR;
        case SDL_SCANCODE_DELETE:       return VK_DELETE;
        case SDL_SCANCODE_END:          return VK_END;
        case SDL_SCANCODE_PAGEDOWN:     return VK_NEXT;
        case SDL_SCANCODE_RIGHT:        return VK_RIGHT;
        case SDL_SCANCODE_LEFT:         return VK_LEFT;
        case SDL_SCANCODE_DOWN:         return VK_DOWN;
        case SDL_SCANCODE_UP:           return VK_UP;
        case SDL_SCANCODE_NUMLOCKCLEAR: return VK_NUMLOCK;
        case SDL_SCANCODE_KP_DIVIDE:    return VK_DIVIDE;
        case SDL_SCANCODE_KP_MULTIPLY:  return VK_MULTIPLY;
        case SDL_SCANCODE_KP_MINUS:     return VK_SUBTRACT;
        case SDL_SCANCODE_KP_PLUS:      return VK_ADD;
        case SDL_SCANCODE_KP_ENTER:     return VK_RETURN;
        case SDL_SCANCODE_KP_1:         return VK_NUMPAD1;
        case SDL_SCANCODE_KP_2:         return VK_NUMPAD2;
        case SDL_SCANCODE_KP_3:         return VK_NUMPAD3;
        case SDL_SCANCODE_KP_4:         return VK_NUMPAD4;
        case SDL_SCANCODE_KP_5:         return VK_NUMPAD5;
        case SDL_SCANCODE_KP_6:         return VK_NUMPAD6;
        case SDL_SCANCODE_KP_7:         return VK_NUMPAD7;
        case SDL_SCANCODE_KP_8:         return VK_NUMPAD8;
        case SDL_SCANCODE_KP_9:         return VK_NUMPAD9;
        case SDL_SCANCODE_KP_0:         return VK_NUMPAD0;
        case SDL_SCANCODE_KP_PERIOD:    return VK_OEM_PERIOD;
        case SDL_SCANCODE_APPLICATION:  return VK_LMENU;
        case SDL_SCANCODE_KP_EQUALS:    goto unsupp;
        case SDL_SCANCODE_F13:          return VK_F13;
        case SDL_SCANCODE_F14:          return VK_F14;
        case SDL_SCANCODE_F15:          return VK_F15;
        case SDL_SCANCODE_F16:          return VK_F16;
        case SDL_SCANCODE_F17:          return VK_F17;
        case SDL_SCANCODE_F18:          return VK_F18;
        case SDL_SCANCODE_F19:          return VK_F19;
        case SDL_SCANCODE_F20:          return VK_F20;
        case SDL_SCANCODE_F21:          return VK_F21;
        case SDL_SCANCODE_F22:          return VK_F22;
        case SDL_SCANCODE_F23:          return VK_F23;
        case SDL_SCANCODE_F24:          return VK_F24;

        case SDL_SCANCODE_LCTRL:        return VK_CONTROL;
        case SDL_SCANCODE_LSHIFT:       return VK_SHIFT;
        case SDL_SCANCODE_LALT:         return VK_MENU;
        case SDL_SCANCODE_LGUI:         return VK_LWIN;
        case SDL_SCANCODE_RCTRL:        return VK_CONTROL;
        case SDL_SCANCODE_RSHIFT:       return VK_SHIFT;
        case SDL_SCANCODE_RALT:         return VK_MENU;
        case SDL_SCANCODE_RGUI:         return VK_RWIN;

        default:
        {
            unsupp:
            uw_log("Unknown sdl scancode (%d) ignored\n", (unsigned)scancode);
            return 0;
        }
    }

}

// mapping from real input to game input should be done here (adding gamepad support and that stuff)

static int translate_SDL_Event(SDL_Event* event, uw_gmq_msg_t* result_message) {
    // only one-to-one or one-to-none. can't do one-to-many events
    if (event->type == sdl_gmq_event) {
        result_message->message = event->user.code;
        result_message->param1 = event->user.data1;
        result_message->param2 = event->user.data2;
        return 0;
    } else {
        switch (event->type) {
            case SDL_QUIT:
                result_message->message = UW_GM_QUIT;
                return 0;
            case SDL_KEYDOWN:
            {
                if (event->key.repeat)
                    return -1;
                int gmq_key = sdl_scancode_to_gmq_key(event->key.keysym.scancode);
                result_message->message = UW_GM_KEYDOWN;
                result_message->param1 = (void*)(uintptr_t)gmq_key;
                return 0;
            }
            case SDL_KEYUP:
            {
                if (event->key.repeat)
                    return -1;
                int gmq_key = sdl_scancode_to_gmq_key(event->key.keysym.scancode);
                result_message->message = UW_GM_KEYUP;
                result_message->param1 = (void*)(uintptr_t)gmq_key;
                return 0;
            }
            case SDL_MOUSEMOTION:
            {
                result_message->message = UW_GM_MOUSEMOVE;
                result_message->param1 = (void*)(uintptr_t)(
                        (event->motion.state & SDL_BUTTON_LMASK ? MK_LBUTTON : 0)
                        | (event->motion.state & SDL_BUTTON_MMASK ? MK_MBUTTON : 0)
                        | (event->motion.state & SDL_BUTTON_RMASK ? MK_RBUTTON : 0)
                );
                result_message->param2 = (void*)(uintptr_t)(((uint16_t)event->motion.x) | (((uint16_t)event->motion.y) << 16));
                return 0;
            }
            case SDL_MOUSEBUTTONUP:
            case SDL_MOUSEBUTTONDOWN:
            {
                int down = event->type == SDL_MOUSEBUTTONDOWN;
                switch (event->button.button)
                {
                    case SDL_BUTTON_LEFT:
                        result_message->message = down ? UW_GM_MOUSELDOWN : UW_GM_MOUSELUP;
                        break;
                    case SDL_BUTTON_RIGHT:
                        result_message->message = down ? UW_GM_MOUSERDOWN : UW_GM_MOUSERUP;
                        break;
                    default:
                        return -1;
                }

                result_message->param1 = (void*)(uintptr_t)(
                        (event->motion.state & SDL_BUTTON_LMASK ? MK_LBUTTON : 0)
                        | (event->motion.state & SDL_BUTTON_MMASK ? MK_MBUTTON : 0)
                        | (event->motion.state & SDL_BUTTON_RMASK ? MK_RBUTTON : 0)
                );
                result_message->param2 = (void*)(uintptr_t)(((uint16_t)event->motion.x) | (((uint16_t)event->motion.y) << 16));
                return 0;
            }
            default:
                uw_log("ignoring unknown event type: %d\n", event->type);
                return -1;
        }
    }
}

void uw_gmq_poll(uw_gmq_msg_t* result_message) {
    SDL_Event event;
    memset(result_message, 0, sizeof(uw_gmq_msg_t));
    while (1) {
        int r = SDL_WaitEvent(&event);
        assert(r != 0);

        if (event.type == sdl_surf_request_event) {
            surf_request* rq = (surf_request*)event.user.data1;
            rq->actually_execute();
        } else {
            if (!translate_SDL_Event(&event, result_message))
                return;
        }
    }
}


/*
 * surf stuff
 */

static void finish_surf_processing(uw_surf_t* surf) {
    char filename[80];
    //sprintf(filename, "sdl_dump/%p_%08d.bpm", surf, bmp_id++);
    //SDL_SaveBMP(primary_surface, filename);

    if (interactive_mode) {
        if (surf == (uw_surf_t *) primary_surface) {
            SDL_UpdateWindowSurface(window);
        }
    }
}

uw_surf_t* uw_ui_surf_alloc(int32_t width, int32_t height) {
    if (!pthread_equal(pthread_self(), main_thread)) {
        alloc_surf_request rq;
        rq.width = width;
        rq.height = height;
        rq.execute_remote();
        return rq.res;
    }

    // we want to manually allocate surface for it to be accesible by guest

    size_t pitch = UW_ALIGN_UP(width, 4) * 2;
    size_t sz = UW_ALIGN_UP(pitch * height, uw_host_page_size);
    uint32_t v = uw_target_map_memory_dynamic(sz, UW_PROT_RW);
    assert(v != (uint32_t) -1);
    void *mem = g2h(v);

    SDL_Surface *res = SDL_CreateRGBSurfaceWithFormatFrom(mem, width, height, 16, pitch, SDL_PIXELFORMAT_RGB565);
    assert(res != NULL);
    SDL_SetSurfaceRLE(res, 0);
    return (uw_surf_t *) res;
}

void uw_ui_surf_free(uw_surf_t* surf) {
    if (surf == (uw_surf_t *) primary_surface)
        return;

    if (!pthread_equal(pthread_self(), main_thread)) {
        free_surf_request rq;
        rq.surf = surf;
        rq.execute_remote();
        return;
    }

    void *pixdata = surf->sdl_surf.pixels;
    size_t pitch = UW_ALIGN_UP(surf->sdl_surf.w, 4);
    size_t sz = UW_ALIGN_UP(pitch * surf->sdl_surf.h * 2, uw_host_page_size);
    uw_unmap_memory(h2g(pixdata), sz);
    SDL_FreeSurface(&surf->sdl_surf);
}

void uw_ui_surf_lock(uw_surf_t* surf, uw_locked_surf_desc_t* locked) {
    if (!pthread_equal(pthread_self(), main_thread)) {
        lock_surf_request rq;
        rq.surf = surf;
        rq.locked = locked;
        rq.execute_remote();
        return;
    }

    //assert(&surf->sdl_surf != primary_surface); // the pointer to data would be not in the guest address space
    int r = SDL_LockSurface(&surf->sdl_surf);
    assert(r == 0);
    locked->data  = surf->sdl_surf.pixels;
    locked->w     = surf->sdl_surf.w;
    locked->h     = surf->sdl_surf.h;
    locked->pitch = surf->sdl_surf.pitch;
}

void uw_ui_surf_unlock(uw_surf_t* surf) {
    if (!pthread_equal(pthread_self(), main_thread)) {
        unlock_surf_request rq;
        rq.surf = surf;
        rq.execute_remote();
        return;
    }

    //  assert(&surf->sdl_surf != primary_surface); // the pointer to data would be not in the guest address space
    SDL_UnlockSurface(&surf->sdl_surf);
    
    finish_surf_processing(surf);
}

static SDL_Rect uw_rect_to_sdl_rect(const uw_rect_t* srect) {
    SDL_Rect rect;
    rect.x = srect->left;
    rect.y = srect->top;
    rect.w = srect->right - srect->left + 1;
    rect.h = srect->bottom - srect->top + 1;
    return rect;
}

void uw_ui_surf_blit(uw_surf_t* src, const uw_rect_t* src_rect, uw_surf_t* dst, uw_rect_t* dst_rect) {
    if (!pthread_equal(pthread_self(), main_thread)) {
        blit_surf_request rq;
        rq.src = src;
        rq.src_rect = src_rect;
        rq.dst = dst;
        rq.dst_rect = dst_rect;
        rq.execute_remote();
        return;
    }
    
    int r = 0;
    
    r = SDL_SetColorKey(&src->sdl_surf, SDL_FALSE, 0);
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
    
    r = SDL_BlitSurface(&src->sdl_surf, psrect, &dst->sdl_surf, pdrect);
    assert(!r);
    
    finish_surf_processing(dst);
}

void uw_ui_surf_blit_srckeyed(uw_surf_t* src, const uw_rect_t* src_rect, uw_surf_t* dst, uw_rect_t* dst_rect, uint16_t srckey_color) {
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

    int r = 0;
    
    r = SDL_SetColorKey(&src->sdl_surf, SDL_TRUE, srckey_color);
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
    
    r = SDL_BlitSurface(&src->sdl_surf, psrect, &dst->sdl_surf, pdrect);
    assert(!r);
    
    finish_surf_processing(dst);
}

void uw_ui_surf_fill(uw_surf_t* dst, uw_rect_t* dst_rect, uint16_t color) {
    if (interactive_mode) {
        if (!pthread_equal(pthread_self(), main_thread)) {
            fill_surf_request rq;
            rq.dst = dst;
            rq.dst_rect = dst_rect;
            rq.color = color;
            rq.execute_remote();
            return;
        }

        if (dst_rect) {
            // TODO: clipping of the dst rectangle is not done
            SDL_Rect rect = uw_rect_to_sdl_rect(dst_rect);
            int r = SDL_FillRect(&dst->sdl_surf, &rect, color);
            assert(!r);
        } else {
            int r = SDL_FillRect(&dst->sdl_surf, NULL, color);
            assert(!r);
        }

        finish_surf_processing(dst);
    }
}

uw_surf_t* uw_ui_surf_get_primary(void) {
    return (uw_surf_t*)primary_surface;
}
