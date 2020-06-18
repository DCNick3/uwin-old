/*
 *  winmm.dll implementation
 *
 *  Copyright (c) 2020 Nikita Strygin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

// hacky hacks around mingw headers
#define _WINMM_

#include <windows.h>
#include <mmreg.h>
#include <ksvc.h>
#include <rpmalloc.h>

MMRESULT WINAPI timeBeginPeriod(UINT uPeriod) {
    kprintf("timeBeginPeriod(%d)", uPeriod);
    return TIMERR_NOERROR;
}

MMRESULT WINAPI timeEndPeriod(UINT uPeriod) {
    kprintf("timeEndPeriod(%d)", uPeriod);
    return TIMERR_NOERROR;
}

DWORD WINAPI timeGetTime() {
    DWORD res = GetTickCount();
    //kprintf("timeGetTime() -> %u", (unsigned)res);
    return res;
}

typedef VOID WINAPI (*waveOutProc)(HWAVEOUT hwo, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2);

#define USE_MUTEX

#ifdef USE_MUTEX
#define LOCK WaitForSingleObject(this->mutex, INFINITE)
#define UNLOCK ReleaseMutex(this->mutex)
#else
#define LOCK EnterCriticalSection(&this->critical_section)
#define UNLOCK LeaveCriticalSection(&this->critical_section)
#endif

//#define log(...) kprintf(__VA_ARGS__)
#define log(...)

typedef struct uw_wave_out {
    khandle_t         kwave_handle;
    uint32_t          kthread_id;
    waveOutProc       user_callback;
    DWORD_PTR         user_callback_instance;
    WAVEHDR          *first_pending_buffer;
    WAVEHDR          *first_enqueued_buffer;
#ifdef USE_MUTEX
    HANDLE            mutex;
#else
    CRITICAL_SECTION  critical_section; // protects linked list
#endif
} uw_wave_out_t;

// here we build a linked list of WAVEHDR's that are queued for playing. kernel will build it too, but using it's own structures.
// to implement messages for waveOutProc we create a thread with sole purpose of calling kwave_poll to wait for new messages.
// it will send "done" and "close" messages, which will be transformed to MM messages.
// no support for looping

static uint32_t __stdcall wave_out_proc(void* param) {
    uw_wave_out_t* this = param;
    
    kprintf("wave_out_proc started");
    this->user_callback(param, WOM_OPEN, this->user_callback_instance, 0, 0);
    
    while (1) {
        uint32_t msg = kwave_wait_message(this->kwave_handle);
        
        //kprintf("kwave message: %d", msg);
        
        switch (msg) {
            case KWAVE_DONE:
            {
                log("KWAVE_DONE...");
                LOCK;
                log("KWAVE_DONE!");
                // remove the buffer from the list
                if (this->first_enqueued_buffer == NULL)
                    kpanic("queue invariant violation");
                
                WAVEHDR *finished_buffer = this->first_enqueued_buffer;
                this->first_enqueued_buffer = this->first_enqueued_buffer->lpNext;
                
                //kprintf("%p finished", finished_buffer);
                
                finished_buffer->dwFlags &= ~WHDR_INQUEUE;
                finished_buffer->dwFlags |= WHDR_DONE;
                UNLOCK;
                
                this->user_callback(param, WOM_DONE, this->user_callback_instance, (DWORD_PTR)finished_buffer, 0);
                
                log("KWAVE_DONE~");
                break;
            }
            case KWAVE_RESET:
            {
                log("KWAVE_RESET");
                LOCK;
                
                while (this->first_enqueued_buffer != NULL) {
                    this->first_enqueued_buffer->dwFlags &= ~WHDR_INQUEUE;
                    this->first_enqueued_buffer = this->first_enqueued_buffer->lpNext;
                }
                
                UNLOCK;
                break;
            }
            case KWAVE_CLOSE:
                log("KWAVE_CLOSE");
                goto close;
            case KWAVE_ENQUEUED:
                log("KWAVE_ENQUEUED...");
                LOCK;
                log("KWAVE_ENQUEUED!");
                if (this->first_pending_buffer == NULL)
                    kpanic("queue invariant violation");
                WAVEHDR *pending_buffer = this->first_pending_buffer;
                
                log("%p enqueued", pending_buffer);
                
                this->first_pending_buffer = pending_buffer->lpNext;
                
                WAVEHDR** next_queued = &this->first_enqueued_buffer;
                while (*next_queued != NULL)
                    next_queued = &((*next_queued)->lpNext);
                *next_queued = pending_buffer;
                pending_buffer->lpNext = NULL;
                
                log("KWAVE_ENQUEUED~");
                UNLOCK;
                break;
        }
    }
    
close:
    this->user_callback(param, WOM_CLOSE, this->user_callback_instance, 0, 0);
    
    return 0;
}

MMRESULT WINAPI waveOutOpen(LPHWAVEOUT phwo, UINT uDeviceID, LPCWAVEFORMATEX pwfx, DWORD_PTR dwCallback, DWORD_PTR dwInstance, DWORD fdwOpen) {
    
    if (fdwOpen != CALLBACK_FUNCTION) {
        kprintf("unsupported fdwOpen");
        goto bad;
    }
    if (dwInstance != 0) {
        kprintf("dwInstance != NULL is not supported");
        goto bad;
    }
    if (uDeviceID != WAVE_MAPPER) {
        kprintf("unsupported uDeviceID");
        goto bad;
    }
    if (pwfx->wFormatTag != WAVE_FORMAT_PCM) {
        kprintf("unsupported format");
        goto bad;
    }
    if (pwfx->nChannels != 2) {
        kprintf("Unsupported nChannels");
        goto bad;
    }
    if (pwfx->nSamplesPerSec != 44100) {
        kprintf("unsupported sample rate");
        goto bad;
    }
    if (pwfx->nAvgBytesPerSec != pwfx->nSamplesPerSec * pwfx->nBlockAlign) {
        kprintf("wrong nAvgBytesPerSec");
        goto bad;
    }
    if (pwfx->nBlockAlign * 8 != pwfx->nChannels * pwfx -> wBitsPerSample) {
        kprintf("wrong nBlockAlign");
        goto bad;
    }
    if (pwfx->wBitsPerSample != 16) {
        kprintf("unsupported wBitsPerSample");
        goto bad;
    }
    
    uw_wave_out_t* this = rpmalloc(sizeof(uw_wave_out_t));
    if (this == NULL)
        kpanic("no memory for uw_wave_out_t");
    
#ifdef USE_MUTEX
    this->mutex = CreateMutexA(NULL, 0, NULL);
#else
    InitializeCriticalSection(&this->critical_section);
#endif
    this->first_pending_buffer = NULL;
    this->user_callback = (waveOutProc)dwCallback;
    this->user_callback_instance = dwInstance;
    this->kwave_handle = kwave_open(pwfx->nSamplesPerSec, pwfx->wBitsPerSample);
    this->kthread_id = kthread_create(wave_out_proc, this, 1024 * 1024 /* 1 MiB; TODO: this can definetely be reduced */, 0);
    
    //kprintf("not implemented yet");
    
    //ksleep(1000000);
    
    *phwo = (HWAVEOUT)this;
    
    kprintf("waveOutOpen(%p, %x, %p { %x %d %d %d %x %d %d }, %p, %p, %x) no-op stub called", phwo, uDeviceID, pwfx, pwfx->wFormatTag, pwfx->nChannels, pwfx->nSamplesPerSec, pwfx->nAvgBytesPerSec, pwfx->nBlockAlign, pwfx->wBitsPerSample, pwfx->cbSize, dwCallback, dwInstance, fdwOpen);
    
    return MMSYSERR_NOERROR;
    
bad:
    kprintf("waveOutOpen(%p, %x, %p { %x %d %d %d %x %d %d }, %p, %p, %x)", phwo, uDeviceID, pwfx, pwfx->wFormatTag, pwfx->nChannels, pwfx->nSamplesPerSec, pwfx->nAvgBytesPerSec, pwfx->nBlockAlign, pwfx->wBitsPerSample, pwfx->cbSize, dwCallback, dwInstance, fdwOpen);
    kpanic("aaa");
}

MMRESULT WINAPI waveOutPrepareHeader(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh) {
    pwh->dwFlags |= WHDR_PREPARED;
    kprintf("waveOutPrepareHeader(%p, %p, %x) called", hwo, pwh, cbwh);
    
    if (cbwh != sizeof(WAVEHDR))
        goto bad;
    
    return MMSYSERR_NOERROR;
bad:
    kpanic("bad waveOutPrepareHeader");
}

MMRESULT WINAPI waveOutUnprepareHeader(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh) {
    pwh->dwFlags &= ~WHDR_PREPARED;
    kprintf("waveOutUnprepareHeader(%p, %p, %x) called", hwo, pwh, cbwh);
    
    if (cbwh != sizeof(WAVEHDR))
        goto bad;
    
    return MMSYSERR_NOERROR;
bad:
    kpanic("bad waveOutUnprepareHeader");
    
}

MMRESULT WINAPI waveOutWrite(HWAVEOUT hwo, LPWAVEHDR pwh, UINT cbwh) {
    uw_wave_out_t* this = (uw_wave_out_t*)hwo;

    if (cbwh != sizeof(WAVEHDR))
        goto bad;
    if (pwh->dwFlags & (WHDR_BEGINLOOP |  WHDR_ENDLOOP) || pwh->dwLoops != 0)
        goto bad;
    
    log("waveOutWrite(%p, %p, %x)...", hwo, pwh, cbwh);
    LOCK;
    log("waveOutWrite(%p, %p, %x)!", hwo, pwh, cbwh);
    
    if (!(pwh->dwFlags & WHDR_PREPARED))
        kpanic("unprepared buffer written to device");
    
    if (pwh->dwFlags & WHDR_INQUEUE)
        kpanic("buffer already present in queue was added");
    
    pwh->dwFlags &= ~WHDR_DONE;
    pwh->dwFlags |=  WHDR_INQUEUE;
    pwh->lpNext = NULL;
    
    int cnt = 0;
    // add buffer to the end of our queue
    WAVEHDR** pbuffer = &this->first_pending_buffer;
    while (*pbuffer != NULL) {
        //kprintf("pending: %p", *pbuffer);
        
        cnt++;
        pbuffer = &((*pbuffer)->lpNext);
 
        
        if (cnt > 1000)
            kpanicf("zalupa");
    }
    *pbuffer = pwh;
    
    log("have %d pending buffers", cnt);
    
    log("waveOutWrite~");
    UNLOCK;
    
    // and inform kernel about it
    
    kwave_add_buffer(this->kwave_handle, pwh->lpData, pwh->dwBufferLength);
    
    
    return MMSYSERR_NOERROR;
bad:
    kpanic("bad waveOutWrite");
}

MMRESULT WINAPI waveOutReset(HWAVEOUT hwo) {
    uw_wave_out_t* this = (uw_wave_out_t*)hwo;
    kprintf("waveOutReset(%p)", hwo);
    kwave_reset(this->kwave_handle);
    return MMSYSERR_NOERROR;
}

MMRESULT WINAPI waveOutGetID(HWAVEOUT hwo, LPUINT puDeviceID) {
    *puDeviceID = 1;
    return MMSYSERR_NOERROR;
}

MMRESULT WINAPI waveOutGetDevCaps(UINT_PTR uDeviceID, LPWAVEOUTCAPS pwoc, UINT cbwoc) {
    
    if (uDeviceID != 1)
        goto bad;
    if (cbwoc != sizeof(WAVEOUTCAPS))
        goto bad;
    
    pwoc->wMid = MM_UNMAPPED;
    pwoc->wPid = MM_PID_UNMAPPED;
    pwoc->vDriverVersion = 0x0100;
    memcpy(pwoc->szPname, "uwin kwave", 11);
    pwoc->dwFormats = WAVE_FORMAT_4S16;
    pwoc->wChannels = 2;
    pwoc->dwSupport = 0;
    
    return MMSYSERR_NOERROR;
bad:
    kpanicf("waveOutGetDevCaps(%x, %p, %d)", uDeviceID, pwoc, cbwoc);
}
