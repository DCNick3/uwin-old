
#include "uwin/uwin.h"
#include "uwin/util/align.h"

#include <stdatomic.h>
#include <assert.h>

#include <SDL.h>
#include <semaphore.h>

// provides gfx, snd & input libraries

struct uw_surf {
    SDL_Surface sdl_surf;
};

static SDL_Window* window;
static SDL_Surface* primary_surface;

void uw_ui_initialize(void) {
    if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO) != 0) {
        uw_log("Unable to initialize SDL: %s", SDL_GetError());
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
    SDL_SetSurfaceRLE(primary_surface, 0);
    SDL_ShowCursor(SDL_DISABLE);
}

void uw_ui_finalize(void) {
    SDL_Window* w = window;
    if (w != NULL)
        SDL_DestroyWindow(w);
    SDL_Quit();
}

//static int bmp_id = 0;

static void finish_surf_processing(uw_surf_t* surf) {
    //char filename[80];
    //sprintf(filename, "sdl_dump/%p_%08d.bpm", surf, bmp_id++);
    //SDL_SaveBMP(primary_surface, filename);
        
    if (surf == (uw_surf_t*)primary_surface) {
        
        SDL_UpdateWindowSurface(window);
    }
}

uw_surf_t* uw_ui_surf_alloc(int32_t width, int32_t height) {
    // we want to manually allocate surface for it to be accesible by guest
    
    size_t pitch = UW_ALIGN_UP(width, 4) * 2;
    size_t sz = UW_ALIGN_UP(pitch * height, uw_host_page_size);
    uint32_t v = uw_target_map_memory_dynamic(sz, UW_PROT_RW);
    assert(v != (uint32_t)-1);
    void* mem = g2h(v);
    
    SDL_Surface* res = SDL_CreateRGBSurfaceWithFormatFrom(mem, width, height, 16, pitch, SDL_PIXELFORMAT_RGB565);
    assert(res != NULL);
    SDL_SetSurfaceRLE(res, 0);
    return (uw_surf_t*)res;
}

void uw_ui_surf_free(uw_surf_t* surf) {
    if (surf == (uw_surf_t*)primary_surface)
        return;
    
    void* pixdata = surf->sdl_surf.pixels;
    size_t pitch = UW_ALIGN_UP(surf->sdl_surf.w, 4);
    size_t sz = UW_ALIGN_UP(pitch * surf->sdl_surf.h * 2, uw_host_page_size);
    uw_unmap_memory(h2g(pixdata), sz);
    SDL_FreeSurface(&surf->sdl_surf);
}

void uw_ui_surf_lock(uw_surf_t* surf, uw_locked_surf_desc_t* locked) {
    int r = SDL_LockSurface(&surf->sdl_surf);
    assert(r == 0);
    locked->data  = surf->sdl_surf.pixels;
    locked->w     = surf->sdl_surf.w;
    locked->h     = surf->sdl_surf.h;
    locked->pitch = surf->sdl_surf.pitch;
}

void uw_ui_surf_unlock(uw_surf_t* surf) {
    SDL_UnlockSurface(&surf->sdl_surf);
    
    finish_surf_processing(surf);
}

static SDL_Rect uw_rect_to_sdl_rect(const uw_rect_t* srect) {
    SDL_Rect rect;
    rect.x = srect->left;
    rect.y = srect->top;
    rect.w = srect->right - srect->left + 1;
    rect.h = srect->bottom - srect->top + 1;
    //qemu_log("rect { %d %d %d %d }\n", rect.x, rect.y, rect.w, rect.h);
    return rect;
}

void uw_ui_surf_blit(uw_surf_t* src, const uw_rect_t* src_rect, uw_surf_t* dst, uw_rect_t* dst_rect) {
    
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

void uw_ui_surf_fill(uw_surf_t* surf, uw_rect_t* dst_rect, uint16_t color) {
    //int r = SDL_LockSurface(&surf->sdl_surf);
    //assert(r == 0);
    
    if (dst_rect) {
        // TODO: clipping of the dst rectangle is not done
        SDL_Rect rect = uw_rect_to_sdl_rect(dst_rect);
        int r = SDL_FillRect(&surf->sdl_surf, &rect, color);
        assert(!r);
    } else {
        int r = SDL_FillRect(&surf->sdl_surf, NULL, color);
        assert(!r);
    }
    
    finish_surf_processing(surf);
    
    /*int pitch = surf->sdl_surf.pitch;
    int x_off = 0, y_off = 0, w = surf->sdl_surf.w, h = surf->sdl_surf.h;
    if (dst_rect != NULL) {
        qemu_log("fill { %d %d %d %d } with %d\n", (int)dst_rect->left, (int)dst_rect->top, (int)dst_rect->right, (int)dst_rect->bottom, (int)color);
        x_off = dst_rect->left;
        w = dst_rect->right - x_off;
        y_off = dst_rect->top;
        h = dst_rect->bottom - y_off;
    }
    else
        qemu_log("fill whole surdace with %d\n", (int)color);
    
    void* data = surf->sdl_surf.pixels + x_off * 2 + y_off * pitch;
    for (int j = 0; j < h; j++)
        for (int i = 0; i < w; i++)
            *(uint16_t*)(data + j * pitch + i * 2) = color;
    
    //assert("Not implemented" == 0);
    SDL_UnlockSurface(&surf->sdl_surf);*/
}

uw_surf_t* uw_ui_surf_get_primary(void) {
    return (uw_surf_t*)primary_surface;
}

/*
typedef struct uw_wave_buffer uw_wave_buffer_t;
struct uw_wave_buffer {
    uint8_t* data;
    uint32_t size;
    uint32_t position;
};


struct uw_wave {
    GQueue *message_queue;
    pthread_cond_t message_cond;
    
    GQueue *buffer_queue;
    
    //pthread_cond_t close_cond;
    pthread_mutex_t mutex;
    SDL_AudioDeviceID sdl_dev;
};

#define PUSH_QUEUE_MESSAGE(msg) { g_queue_push_tail(this->message_queue, GUINT_TO_POINTER(msg)); pthread_cond_signal(&this->message_cond); }

static void my_SDL_AudioCallback(void* userdata, uint8_t* stream, int len) {
    uw_wave_t* this = userdata;
    //qemu_log("my_SDL_AudioCallback...\n");
    pthread_mutex_lock(&this->mutex);
    
    //qemu_log("my_SDL_AudioCallback!\n");
    
    while (len > 0) {
        if (g_queue_is_empty(this->buffer_queue)) {
            //qemu_log("buffer underflow for %d bytes!\n", len);
            memset(stream, 0, len);
            break;
        }
        
        uw_wave_buffer_t *first_buffer = g_queue_peek_head(this->buffer_queue);
        int l = len;
        int rem = first_buffer->size - first_buffer->position;
        if (rem < l)
            l = rem;
        
        memcpy(stream, first_buffer->data + first_buffer->position, l);
        len -= l;
        stream += l;
        first_buffer->position += l;
        
        if (first_buffer->position == first_buffer->size) {
            //qemu_log("buffer is empty. Pushing UW_WAVE_DONE to message queue\n");
            g_slice_free(uw_wave_buffer_t, first_buffer);
            g_queue_pop_head(this->buffer_queue);
            PUSH_QUEUE_MESSAGE(UW_WAVE_DONE);
        }
    }
    
    pthread_mutex_unlock(&this->mutex);
}

uw_wave_t* uw_ui_wave_open(uint32_t sample_rate, uint32_t bits_per_sample) {
    uw_wave_t* this = g_new0(uw_wave_t, 1);
    
    int r;
    
    r = pthread_mutex_init(&this->mutex, NULL);
    assert(!r);
    
    r = pthread_cond_init(&this->message_cond, NULL);
    assert(!r);
    
    this->message_queue = g_queue_new();
    this->buffer_queue = g_queue_new();
    
    SDL_AudioSpec want, have;

    assert(bits_per_sample == 16);
    
    want.freq = sample_rate;
    want.format = AUDIO_S16LSB;
    want.channels = 2;
    want.samples = 1024;
    want.callback = my_SDL_AudioCallback;
    want.userdata = this;
    
    this->sdl_dev = SDL_OpenAudioDevice(NULL, 0, &want, &have, 0);
    if (this->sdl_dev == 0)
        assert("cannot get sdl audio device" == 0);
    
    SDL_PauseAudioDevice(this->sdl_dev, 0);
    
    return this;
}

void uw_ui_wave_close(uw_wave_t* this) {
    
    assert("not implemented" == 0);
    
    SDL_CloseAudioDevice(this->sdl_dev);
    
    pthread_mutex_lock(&this->mutex);
    
    PUSH_QUEUE_MESSAGE(UW_WAVE_CLOSE);
    
    pthread_mutex_unlock(&this->mutex);
    
    // TODO: synchronize stuff with uw_ui_wave_wait_message
}

static void uw_ui_wave_buffer_free(gpointer ptr) {
    g_slice_free(uw_wave_buffer_t, ptr);
}

void uw_ui_wave_reset(uw_wave_t* this) {
    //qemu_log("uw_ui_wave_reset...\n");
    pthread_mutex_lock(&this->mutex);
    //qemu_log("uw_ui_wave_reset!\n");
    
    
    // TODO: this is a pretty new function (glib 2.60). Maybe it's usage can be avoided
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    g_queue_clear_full(this->buffer_queue, uw_ui_wave_buffer_free);
#pragma GCC diagnostic pop
    
    qemu_log("pushing UW_WAVE_RESET\n");
    PUSH_QUEUE_MESSAGE(UW_WAVE_RESET);
    
    pthread_mutex_unlock(&this->mutex);
}

uint32_t uw_ui_wave_wait_message(uw_wave_t* this) {
     //qemu_log("uw_ui_wave_wait_message...\n");
    pthread_mutex_lock(&this->mutex);
    //qemu_log("uw_ui_wave_wait_message!\n");
    
    while (g_queue_is_empty(this->message_queue)) {
        pthread_cond_wait(&this->message_cond, &this->mutex);
        //qemu_log("wait message wakeup...\n");
    }
    
    uint32_t message = GPOINTER_TO_UINT(g_queue_pop_head(this->message_queue));
    
    //if (message == UW_WAVE_RESET)
    
    // TODO: UW_WAVE_CLOSE
    
    pthread_mutex_unlock(&this->mutex);
    
    return message;
}

void uw_ui_wave_add_buffer(uw_wave_t* this, void* data, uint32_t size) {
    //qemu_log("uw_ui_wave_add_buffer...\n");
    pthread_mutex_lock(&this->mutex);
    //qemu_log("uw_ui_wave_add_buffer!\n");
    
    uw_wave_buffer_t* buffer = g_slice_new(uw_wave_buffer_t);
    buffer->data = data;
    buffer->size = size;
    buffer->position = 0;
    
    g_queue_push_tail(this->buffer_queue, buffer);
    PUSH_QUEUE_MESSAGE(UW_WAVE_ENQUEUED);
    
    pthread_mutex_unlock(&this->mutex);
}
*/
