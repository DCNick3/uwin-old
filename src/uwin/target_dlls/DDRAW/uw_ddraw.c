
#include <windows.h>

#include "uw_ddraw.h"

#include <ksvc.h>
#include <rpmalloc.h>

HRESULT WINAPI DirectDraw_CreateSurface(LPDIRECTDRAW iface, LPDDSURFACEDESC pDDSD,
                LPDIRECTDRAWSURFACE *ppSurf,
                IUnknown *pUnkOuter)
{
    uw_DirectDraw_t* this = (uw_DirectDraw_t*)iface;
    
    if (sizeof(DDSURFACEDESC) != pDDSD->dwSize)
        goto bad;
    
    kprintf("SURFACEDESC: { dwFlags = %x, ddsCaps.dwCaps = %x, dwHeight = %d, dwWidth = %d }", pDDSD->dwFlags, pDDSD->ddsCaps.dwCaps, pDDSD->dwHeight, pDDSD->dwWidth);
    
    if ((pDDSD->dwFlags & DDSD_CAPS) == 0) {
        kprintf("unsupported flags");
        goto bad;
    }
    
    int type;
    
    if (pDDSD->ddsCaps.dwCaps == DDSCAPS_PRIMARYSURFACE) {
        type = UW_SURFACE_SCREEN;
    } else if (pDDSD->ddsCaps.dwCaps == (DDSCAPS_SYSTEMMEMORY | DDSCAPS_OFFSCREENPLAIN))  {
        type = UW_SURFACE_BUFFER;
    } else {
        kprintf("unsupported surface caps");
        goto bad;
    }
    
    if (pUnkOuter != NULL)
        goto bad;
    
    if (this->screen_created && type == UW_SURFACE_SCREEN)
        kpanic("attempt to allocate second screen surface");
    
    uw_DirectDrawSurface_t* surf;
    surf = rpmalloc(sizeof(uw_DirectDrawSurface_t));
    if (surf == NULL)
        kpanic("no memory for uw_DirectDrawSurface_t");
    memset(surf, 0, sizeof(uw_DirectDrawSurface_t));
    surf->lpVtbl = &uw_DirectDrawSurface_Vtable;\
    surf->type = type;
    
    if (type == UW_SURFACE_SCREEN) {
        surf->handle = ksurf_get_primary();
        this->screen_created = 1;
    } else {
        if (pDDSD->dwFlags != (DDSD_HEIGHT | DDSD_WIDTH | DDSD_CAPS)) {
            kprintf("weird flags");
            goto bad;
        }
        
        surf->handle = ksurf_alloc(pDDSD->dwWidth, pDDSD->dwHeight);
    }
    
    *ppSurf = (LPDIRECTDRAWSURFACE)surf;
    
    return DD_OK;
    
bad:
    kpanicf("DirectDraw_CreateSurface(%p, %p, %p); size = %d", pDDSD, ppSurf, pUnkOuter, pDDSD->dwSize);
}

HRESULT WINAPI DirectDraw_SetCooperativeLevel(LPDIRECTDRAW iface, HWND hwnd, DWORD dwFlags)
{
    if (hwnd != HWND_DUMMY_GAME)
        goto bad;
    if (dwFlags != (DDSCL_FULLSCREEN | DDSCL_ALLOWREBOOT | DDSCL_EXCLUSIVE) && dwFlags != DDSCL_NORMAL)
        goto bad;
    
    return DD_OK;
    
bad:
    kpanicf("DirectDraw_SetCooperativeLevel(%x, %x)", hwnd, dwFlags);
}

HRESULT WINAPI DirectDraw_SetDisplayMode(LPDIRECTDRAW iface, DWORD dwWidth, DWORD dwHeight, DWORD dwBPP)
{
    if (dwWidth != 800 || dwHeight != 600 || dwBPP != 16)
        goto bad;
    
    return DD_OK;
bad:
    kpanicf("DirectDraw_SetDisplayMode(%d, %d, %d)", dwWidth, dwHeight, dwBPP);
}

HRESULT WINAPI DirectDraw_RestoreDisplayMode(LPDIRECTDRAW iface)
{
    kprintf("DirectDraw_RestoreDisplayMode(%p)", iface);
    
    return DD_OK;
}

ULONG WINAPI DirectDraw_Release(LPDIRECTDRAW iface)
{
    uw_DirectDraw_t* this = (uw_DirectDraw_t*)iface;
    kprintf("DirectDraw_Release(%p)", iface);
    rpfree(this);
    return 0;
}
