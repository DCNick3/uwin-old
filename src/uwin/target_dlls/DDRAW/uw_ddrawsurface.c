
#include <windows.h>

#include "uw_ddraw.h"

#include <ksvc.h>
#include <rpmalloc.h>

#define GET_THIS uw_DirectDrawSurface_t* this = (uw_DirectDrawSurface_t*)iface

HRESULT WINAPI DirectDrawSurface_GetPixelFormat(LPDIRECTDRAWSURFACE iface, LPDDPIXELFORMAT lpDDPixelFormat) {
    //kprintf("DirectDrawSurface_GetPixelFormat(%p); size = %d", lpDDPixelFormat, lpDDPixelFormat->dwSize);
    
    if (lpDDPixelFormat->dwSize != sizeof(DDPIXELFORMAT))
        goto bad;
    
    memset(lpDDPixelFormat, 0, sizeof(DDPIXELFORMAT));
    
    lpDDPixelFormat->dwSize = sizeof(DDPIXELFORMAT);
    
    // RGB565
    lpDDPixelFormat->dwFlags = DDPF_RGB;
    lpDDPixelFormat->dwRGBBitCount      = 16;
    lpDDPixelFormat->dwRBitMask         = 0xf800;
    lpDDPixelFormat->dwGBitMask         = 0x07e0;
    lpDDPixelFormat->dwBBitMask         = 0x001f;
    lpDDPixelFormat->dwRGBAlphaBitMask  = 0x000;
    
    return DD_OK;
    
bad:
    kpanicf("DirectDrawSurface_GetPixelFormat(%p); size = %d", lpDDPixelFormat, lpDDPixelFormat->dwSize);
}

HRESULT WINAPI DirectDrawSurface_Lock(LPDIRECTDRAWSURFACE iface, LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent) {
    GET_THIS;
    if (lpDestRect != NULL)
        goto bad;
    
    if (this->type == UW_SURFACE_SCREEN) {
        kprint("cannot lock screen surface");
        goto bad;
    }
    
    if (this->locked) {
        kprintf("already locked");
        goto bad;
    }
    
    if (lpDDSurfaceDesc->dwSize != sizeof(DDSURFACEDESC))
        goto bad;
    
    if (dwFlags != DDLOCK_WAIT)
        goto bad;
    
    ksurf_lockdesc_t lock_desc;
    ksurf_lock(this->handle, &lock_desc);
    
    //kprintf("DirectDrawSurface_Lock(%p, %p, %p, %x, %p); type = %d, size = %d", iface, lpDestRect, lpDDSurfaceDesc, dwFlags, hEvent, this->type, lpDDSurfaceDesc->dwSize);
    //kprintf("lockdesc { %p, %d, %d, %d }", lock_desc.data, lock_desc.pitch, lock_desc.w, lock_desc.h);
    //kprintf("retaddr = %x", __builtin_return_address(0));
    
    lpDDSurfaceDesc->dwFlags = DDSD_PIXELFORMAT | DDSD_PITCH | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS; // microsoft does not set DDSD_LPSURFACE here... Weird flex, but okay
    lpDDSurfaceDesc->dwHeight = lock_desc.h;
    lpDDSurfaceDesc->dwWidth = lock_desc.w;
    lpDDSurfaceDesc->lPitch = lock_desc.pitch;
    lpDDSurfaceDesc->lpSurface = lock_desc.data;
    lpDDSurfaceDesc->ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN;
    
    lpDDSurfaceDesc->ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    DirectDrawSurface_GetPixelFormat(iface, &lpDDSurfaceDesc->ddpfPixelFormat);
    
    this->locked = 1;
    
    return DD_OK;
    
bad:
    kpanicf("DirectDrawSurface_Lock(%p, %p, %p, %x, %p); type = %d, size = %d", iface, lpDestRect, lpDDSurfaceDesc, dwFlags, hEvent, this->type, lpDDSurfaceDesc->dwSize);
}

HRESULT WINAPI DirectDrawSurface_Unlock(LPDIRECTDRAWSURFACE iface, LPVOID lpSurfaceData) {
    GET_THIS;
    if (!this->locked) {
        kprintf("not locked");
        goto bad;
    }
    
    ksurf_unlock(this->handle);
    this->locked = 0;
    
    //kprintf("DirectDrawSurface_Unlock(%p, %p); type = %d, handle = %x", iface, lpSurfaceData, this->type, this->handle);
    
    return DD_OK;
    
bad:
    kpanicf("DirectDrawSurface_Unlock(%p, %p); type = %d, handle = %x", iface, lpSurfaceData, this->type, this->handle);
}

HRESULT WINAPI DirectDrawSurface_SetColorKey(LPDIRECTDRAWSURFACE iface, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey) {
    GET_THIS;
    if (dwFlags != DDCKEY_SRCBLT) {
        goto bad;
    }
    if (lpDDColorKey->dwColorSpaceLowValue != 0x7ff || lpDDColorKey->dwColorSpaceHighValue != 0x7ff)
        goto bad;
    
    this->colorkey = lpDDColorKey->dwColorSpaceLowValue;
    this->has_bltsrc_colorkey = 1;
    
    return DD_OK;
    
bad:
    kpanicf("DirectDrawSurface_SetColorKey(%p, %x, %p); type = %d; lo = %x; hi = %x", iface, dwFlags, lpDDColorKey, this->type,
            lpDDColorKey->dwColorSpaceLowValue, lpDDColorKey->dwColorSpaceHighValue);
}

#define RPRNT(r) r left, r top, r right, r bottom

HRESULT WINAPI DirectDrawSurface_Blt(LPDIRECTDRAWSURFACE iface, LPRECT lpDestRect, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx) {
    GET_THIS;
    uw_DirectDrawSurface_t* source = (uw_DirectDrawSurface_t*)lpDDSrcSurface;
    
    int use_color_key = -1;
    
        
    //kprintf("DirectDrawSurface_Blt(%p, %p, %p, %p, %x, %p)", iface, lpDestRect, lpDDSrcSurface, lpSrcRect, dwFlags, lpDDBltFx);
        
    if (dwFlags & ~(DDBLT_COLORFILL | DDBLT_KEYSRC) != 0) {
        kprintf("Unknown flags");
        goto bad;
    }
    
    if (dwFlags & DDBLT_COLORFILL) {
        //kprintf("doing DDBLT_COLORFILL as no-op");
        
        
        //kprintf("DirectDrawSurface_Blt(%p, %p, %p, %x, %p) executed as no-op", iface, lpDestRect, lpDDSrcSurface, dwFlags, lpDDBltFx);
        ksurf_fill(this->handle, (krect_t*)lpDestRect, lpDDBltFx->dwFillColor);
        
        return DD_OK;
    } else {
        //if (dwFlags & DDBLT_KEYSRCOVERRIDE && lpDDBltFx == NULL)
        //    use_color_key = 0;
        //else 
        //kprintf("DirectDrawSurface_Blt(%p, %p { %d %d %d %d }, %p, %p { %d %d %d %d }, %x, %p)", iface, lpDestRect, RPRNT(lpDestRect->), lpDDSrcSurface, lpSrcRect, RPRNT(lpSrcRect->), dwFlags, lpDDBltFx);
        use_color_key = source->has_bltsrc_colorkey;
        /*
        if (dwFlags & DDBLT_KEYSRC) {
                //kpanic("wut?");
            if (source->has_bltsrc_colorkey == 0) {
                kprintf("wut?");
                goto bad;
            }
            use_color_key = 1;
        } else if (source->has_bltsrc_colorkey == 0) {
            use_color_key = 0;
        }*/
        if (use_color_key == -1) {
            kprintf("color key?");
            goto bad;
        }
        
        if (!use_color_key) {
            ksurf_blit(source->handle, (krect_t*)lpSrcRect, this->handle, (krect_t*)lpDestRect);
        } else {
            ksurf_blit_srckeyed(source->handle, (krect_t*)lpSrcRect, this->handle, (krect_t*)lpDestRect, this->colorkey);
        }
        
        
        return DD_OK;
    }
    
bad:
    //kprintf("src type = %d; src handle = %x; src has_colorkey = %d; dst type = %d; dst handle = %x; dst has_colorkey = %d", source->type, source->handle, source->has_bltsrc_colorkey, this->type, this->handle, this->has_bltsrc_colorkey);
    kpanicf("DirectDrawSurface_Blt(%p, %p { %d %d %d %d }, %p, %p { %d %d %d %d }, %x, %p)", iface, lpDestRect, RPRNT(lpDestRect->), lpDDSrcSurface, lpSrcRect, RPRNT(lpSrcRect->), dwFlags, lpDDBltFx);
}

HRESULT WINAPI DirectDrawSurface_GetSurfaceDesc(LPDIRECTDRAWSURFACE iface, LPDDSURFACEDESC lpDDSurfaceDesc) {
    GET_THIS;
    lpDDSurfaceDesc->dwFlags = DDSD_PIXELFORMAT | DDSD_WIDTH | DDSD_HEIGHT | DDSD_CAPS;
    
    ksurf_lockdesc_t lock_desc;
    ksurf_lock(this->handle, &lock_desc);
    ksurf_unlock(this->handle); // not very effective, but meh...
    
    lpDDSurfaceDesc->dwHeight = lock_desc.h;
    lpDDSurfaceDesc->dwWidth = lock_desc.w;
    if (this->type == UW_SURFACE_BUFFER) {
        lpDDSurfaceDesc->ddsCaps.dwCaps = DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN;
        kpanic("what are buffer caps again?");
    }
    else if (this->type == UW_SURFACE_SCREEN)
        lpDDSurfaceDesc->ddsCaps.dwCaps = DDCAPS_GDI | DDCAPS_OVERLAY | DDCAPS_PALETTE;
        
    lpDDSurfaceDesc->ddpfPixelFormat.dwSize = sizeof(DDPIXELFORMAT);
    DirectDrawSurface_GetPixelFormat(iface, &lpDDSurfaceDesc->ddpfPixelFormat);
    
    return DD_OK;
    
bad:
    kpanicf("DirectDrawSurface_GetSurfaceDesc(%p, %p), type = %d, handle = %x", iface, lpDDSurfaceDesc, this->type, this->handle);
}

ULONG WINAPI DirectDrawSurface_Release(LPDIRECTDRAWSURFACE iface)
{
    GET_THIS;
    kprintf("DirectDrawSurface_Release(%p)", iface);
    kht_delref(this->handle, NULL, NULL);
    rpfree(this);
    return 0;
}
