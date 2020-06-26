
#include <windows.h>
#include <ksvc.h>
#include <rpmalloc.h>

//BinkOpenMiles

#define BINK __declspec(dllexport) __stdcall

void BinkOpenMiles(void);

int32_t BINK BinkSetSoundSystem(void* sound_system, void* arg) {
    
    if (sound_system != BinkOpenMiles)
        goto bad;
    
    kprintf("mss driver is %p", arg);
    
    return 1;
    
bad:
    kpanicf("BinkSetSoundSystem(%p, %p)", sound_system, arg);
}

int32_t BINK BinkDDSurfaceType(void* dd_surface) {
    kprintf("BinkDDSurfaceType(%p) -> 0", dd_surface);
    return 0;
}

typedef struct {
    uint32_t width;             // 0
    uint32_t height;            // 4
    uint32_t total_frames;      // 8
    uint32_t frame;             // 12
    uint32_t unk1;              // 16
    uint32_t unk2;              // 20
    uint32_t unk3;              // 24
    uint32_t unk4;              // 28
    uint32_t unk5;              // 32
    uint32_t unk6;              // 36
    uint32_t unk7;              // 40
    uint32_t unk8;              // 44
    
    RECT redraw_rects[8];       // 48
    uint32_t redraw_rect_count; // 176
    uint32_t unk10;             // 180
    uint32_t unk11;             // 184
    uint32_t unk12;             // 188
    
    uint32_t unk13;             // 192
    uint32_t unk14;             // 196
    uint32_t unk15;             // 200
    uint32_t unk16;             // 204
    uint32_t unk17;             // 208
    uint32_t unk18;             // 212
    uint32_t unk19;             // 216
    uint32_t unk20;             // 220
} bink_t;

typedef struct {
    uint32_t unk1;
    uint32_t unk2;
    uint32_t total_time;
    uint32_t unk3;
    uint32_t unk4;
    uint32_t frame_rate;
    uint32_t unk5;
    uint32_t unk6;
    uint32_t total_frames;
    uint32_t unk7;
    uint32_t unk8;
    uint32_t unk9;
    uint32_t sound_underruns;
    uint32_t blit_time;
    uint32_t read_time;
} bink_summary_t;

bink_t* BINK BinkOpen(const char* name, uint32_t flags) {
    if (flags != 0x8000000 
            && flags != 0x8400000) // preload?
        goto bad;
    
    kprintf("BinkOpen(%p, %x)", name, flags);
    
    bink_t* this = rpmalloc(sizeof(bink_t));
    memset(this, 0, sizeof(bink_t));
    this->width = 400;
    this->height = 300;
    this->total_frames = 10;
    this->frame = 1;
    
    return this;
    
bad:
    kpanicf("BinkOpen(%p, %x)", name, flags);
}

uint32_t BINK BinkWait(bink_t* this) {
    kprintf("BinkWait(%p) -> 0", this);
    return 0;
}

int32_t BINK BinkDoFrame(bink_t* this) {
    kprintf("BinkDoFrame(%p) -> 0", this);
    return 0;
}

int32_t BINK BinkCopyToBuffer(bink_t* this, void* buffer, int32_t pitch, int32_t height, int32_t x, int32_t y, int32_t buf_format) {
    kprintf("BinkCopyToBuffer(%p, %p, %d, %d, %d, %d, %d)", this, buffer, pitch, height, x, y, buf_format);
    return 0;
}

void BINK BinkNextFrame(bink_t* this) {
    kprintf("BinkNextFrame(%p) (frame = %d/%d)", this, this->frame, this->total_frames);
    this->frame = ((this->frame) % this->total_frames) + 1;
}

int32_t BINK BinkGetRects(bink_t* this, uint32_t unk) {
    kprintf("BinkGetRects(%p, %x)", this, unk);
    this->redraw_rects[0].left = 0;
    this->redraw_rects[0].top = 0;
    this->redraw_rects[0].right = this->width;
    this->redraw_rects[0].bottom = this->height;
    this->redraw_rect_count = 1;
    
    return 1;
}

void BINK BinkGetSummary(bink_t* this, bink_summary_t* summary) {
    kprintf("BinkGetSummary(%p, %p)", this, summary);
    
    memset(summary, 0, sizeof(summary));
    
    summary->total_time = 1;
    summary->frame_rate = 1;
    summary->total_frames = 1;
    summary->sound_underruns = 0;
    summary->blit_time = 0;
    summary->read_time = 0;
    
}

int32_t BINK BinkPause(bink_t* this, int32_t pause) {
    kprintf("BinkPause(%p, %d)", this, pause);
    return 0;
}

void BINK BinkClose(bink_t* this) {
    kprintf("BinkClose(%p)", this);
    rpfree(this);
}

void BINK BinkGoto(bink_t* this, uint32_t frame, uint32_t unk1) {
    kprintf("BinkGoto(%p, %u, %x)", this, frame, unk1);
}
