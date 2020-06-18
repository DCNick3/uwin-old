
#include <windows.h>
#include <ksvc.h>
#include <rpmalloc.h>

#define MSS __declspec(dllexport) __stdcall

// races may happen.. But meh
static int driver_created = 0;

int32_t MSS AIL_startup(void) {
    kprintf("AIL_startup()");
    return 0;
}

int32_t MSS AIL_set_preference(uint32_t number, int32_t value) {
    kprintf("AIL_set_preference(%u, %d)", number, value);
    
    if (number == 15) // use waveout
        return 0;
    if (number == 33) // use mixer
        return 0;
    if (number == 34) // fragment size
        return 0;
bad:
    kpanicf("AIL_set_preference(%u, %d)", number, value);
}

HWND MSS AIL_HWND(void) {
    HWND res = (HWND)1; // dummy
    
    kprintf("AIL_HWND() -> %p", res);
    
    return res;
}

typedef struct {
    int well;
} driver_t;

#define NUM_USERDATA 1

typedef struct {
    driver_t* driver;
    int32_t sample_rate;
    
    int32_t userdata[NUM_USERDATA];
    
    int32_t volume;
    int32_t pan;
} sample_t;

typedef struct {
    driver_t* driver;
} stream_t;

int32_t MSS AIL_waveOutOpen(driver_t** driver, void* unk1, int32_t unk2, LPWAVEFORMAT lpFormat) {
    kprintf("AIL_waveOutOpen(%p, %p, %d, %p)", driver, unk1, unk2, lpFormat);
    
    if (driver_created)
        kpanicf("driver already created");
    driver_created = 1;
    
    if (unk1 != 0 || unk2 != -1 || lpFormat == NULL)
        goto bad;
    
    if (lpFormat->wFormatTag != WAVE_FORMAT_PCM || lpFormat->nChannels != 2 || lpFormat->nSamplesPerSec != 44100 || lpFormat->nAvgBytesPerSec != 176400 || lpFormat->nBlockAlign != 4) {
        kprintf("bad format");
        goto bad;
    }
    
    *driver = rpmalloc(sizeof(driver_t));
    if (*driver == NULL) {
        kprintf("no mem for driver_t");
        goto bad;
    }
    
    kprintf("driver handle is %p", *driver);
    
    return 0;
    
bad:
    kpanicf("AIL_waveOutOpen(%p, %p, %d, %p)", driver, unk1, unk2, lpFormat);
}

void MSS AIL_digital_configuration(driver_t* driver, void* unk1, void* unk2, char* name_buffer) {
    
    kprintf("AIL_digital_configuration(%p, %p, %p, %p)", driver, unk1, unk2, name_buffer);
    
    if (unk1 != NULL || unk2 != NULL)
        goto bad;
    
    strcat(name_buffer, "DirectSound - MSS Mixer");
    
    return;
    
bad:
    kpanicf("AIL_digital_configuration(%p, %p, %p, %p)", driver, unk1, unk2, name_buffer);
}

int32_t MSS AIL_get_preference(uint32_t number) {
    kprintf("AIL_get_preference(%u)", number);
    
    if (number == 15) // use waveout
        return 1;
    
bad:
    kpanicf("AIL_get_preference(%u)", number);
}

sample_t* MSS AIL_allocate_sample_handle(driver_t* driver) {
    kprintf("AIL_allocate_sample_handle(%p)", driver);
    
    sample_t *r = rpmalloc(sizeof(sample_t));
    if (r == NULL)
        kpanicf("no mem for AIL_allocate_sample_handle");
    
    r->driver = driver;
    r->sample_rate = 44100;
    
    return r;
}

void MSS AIL_serve(void) {
    kprintf("AIL_serve()");
}

int32_t MSS AIL_minimum_sample_buffer_size(driver_t* driver, int32_t rate, int32_t format) {
    
    if (rate == 22050 && (format == 3 || format == 2))
        return 4096;
    
    kpanicf("AIL_minimum_sample_buffer_size(%p, %d, %d)", driver, rate, format);
}


void* MSS AIL_mem_alloc_lock(uint32_t size) {
    void* res = rpmalloc(size);
    if (res == NULL)
        kpanicf("AIL_mem_alloc_lock is out of memory");
    kprintf("AIL_mem_alloc_lock(%d) -> %p", size, res);
    return res;
}

void MSS AIL_mem_free_lock(void* ptr) {  
    kprintf("AIL_mem_free_lock(%p)", ptr);
    rpfree(ptr);
}

void MSS AIL_init_sample(sample_t* sample) {
    kprintf("AIL_init_sample(%p)", sample);
}

void MSS AIL_set_sample_type(sample_t* sample, int32_t type, uint32_t flags) {
    if (!((type == 3 || flags == 1) || (type == 2 || flags == 0)))
        goto bad;
    
    return;
    
bad:
    kpanicf("AIL_set_sample_type(%p, %d, %x)", sample, type, flags);
}

void MSS AIL_set_sample_playback_rate(sample_t* sample, int32_t sample_rate) {
    kprintf("AIL_set_sample_playback_rate(%p, %d)", sample, sample_rate);
    sample->sample_rate = sample_rate;
}

void MSS AIL_set_sample_user_data(sample_t* sample, uint32_t index, int32_t value) {
    kprintf("AIL_set_sample_user_data(%p, %u, %d)", sample, index, value);
    if (index >= NUM_USERDATA)
        kpanicf("userdata index %d is out of range", index);
    
    sample->userdata[index] = value;
}

void MSS AIL_set_sample_volume(sample_t* sample, int32_t volume) {
    kprintf("AIL_set_sample_volume(%p, %d)", sample, volume);
    sample->volume = volume;
}

void MSS AIL_set_sample_pan(sample_t* sample, int32_t pan) {
    kprintf("AIL_set_sample_pan(%p, %d)", sample, pan);
    sample->pan = pan;
}

int32_t MSS AIL_sample_buffer_info(sample_t* sample, uint32_t *unk1, uint32_t *unk2, uint32_t *unk3, uint32_t *unk4) {
    
    kprintf("AIL_sample_buffer_info(%p, %p, %p, %p, %p)", sample, unk1, unk2, unk3, unk4);
    
    if (unk1 != NULL || unk2 != NULL || unk3 != NULL || unk4 != NULL)
        goto bad;
    
    return 1;
    
bad:
    kpanicf("AIL_sample_buffer_info(%p, %p, %p, %p, %p)", sample, unk1, unk2, unk3, unk4);
}

int32_t MSS AIL_sample_buffer_ready(sample_t* sample) {
    kprintf("AIL_sample_buffer_ready(%p) -> 0", sample);
    return 0;
}

void MSS AIL_load_sample_buffer(sample_t* sample, uint32_t buf_num, void* buffer, uint32_t size) {
    kprintf("AIL_load_sample_buffer(%p, %d, %p, %x)", sample, buf_num, buffer, size);
}

void MSS AIL_end_sample(sample_t* sample) {
    kprintf("AIL_end_sample(%p)", sample);
}

void MSS AIL_release_sample_handle(sample_t* sample) {
    rpfree(sample);
}

stream_t* MSS AIL_open_stream(driver_t* driver, const char* filename, int32_t unk) {
    kprintf("AIL_open_stream(%p, %s, %d)", driver, filename, unk);
    
    if (unk != 0)
        goto bad;
    
    stream_t* res = rpmalloc(sizeof(stream_t));
    if (res == NULL)
        kpanic("no mem for stream_t");
    
    res->driver = driver;
    
    return res;
    
bad:
    kpanicf("AIL_open_stream(%p, %s, %d)", driver, filename, unk);
}

void MSS AIL_set_stream_volume(stream_t* stream, int32_t volume) {
    kprintf("AIL_set_stream_volume(%p, %d)", stream, volume);
}

void MSS AIL_set_stream_loop_count(stream_t* stream, int32_t count) {
    kprintf("AIL_set_stream_loop_count(%p, %d)", stream, count);
}

int32_t MSS AIL_service_stream(stream_t* stream, int32_t unk) {
    kprintf("AIL_service_stream(%p, %d)", stream, unk);
    return 0;
}

void MSS AIL_start_stream(stream_t* stream) {
    kprintf("AIL_start_stream(%p)", stream);
}

int32_t MSS AIL_sample_status(sample_t* sample) {
    kprintf("AIL_sample_status(%p) -> 2", sample);
    return 0x2; // done
}

int32_t MSS AIL_set_sample_file(sample_t* sample, void* data, uint32_t unk) {
    kprintf("AIL_set_sample_file(%p, %p, %u) -> 0", sample, data, unk);
    return 0;
}

void MSS AIL_set_sample_loop_count(sample_t* sample, int32_t loop_count) {
    kprintf("AIL_set_sample_loop_count(%p, %d)", sample, loop_count);
}

void MSS AIL_start_sample(sample_t* sample) {
    kprintf("AIL_start_sample(%p)", sample);
}

int32_t MSS AIL_stream_status(stream_t* stream) {
    kprintf("AIL_stream_status(%p) -> 4", stream);
    return 4; //playing
}

int32_t MSS AIL_stream_position(stream_t* stream) {
    kprintf("AIL_stream_position(%p)", stream);
    return 0;
}

int32_t MSS AIL_stream_volume(stream_t* stream) {
    kprintf("AIL_stream_volume(%p) -> 128", stream);
    return 128;
}

void MSS AIL_pause_stream(stream_t* stream, int32_t unk) {
    kprintf("AIL_pause_stream(%p, %d)", stream, unk);
}

void MSS AIL_close_stream(stream_t* stream) {
    kprintf("AIL_close_stream(%p)", stream);
}

void MSS AIL_shutdown(void) {
    kprintf("AIL_shutdown()");
}

void MSS AIL_set_stream_position(stream_t* stream, int32_t position) {
    kprintf("AIL_set_stream_position(%p, %d)", stream, position);
}
