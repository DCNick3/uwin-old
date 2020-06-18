
#include <windows.h>

#include "uw_ddraw.h"

#include <ksvc.h>

#define GET_THIS uw_DirectDrawSurface_t* this = (uw_DirectDrawSurface_t*)iface

HRESULT WINAPI DirectDrawSurface_QueryInterface(LPDIRECTDRAWSURFACE iface, REFIID riid, void** ppvObject) {
    kpanic("DirectDrawSurface_QueryInterface stub called");
}
ULONG WINAPI DirectDrawSurface_AddRef(LPDIRECTDRAWSURFACE iface)
{
    kpanicf("DirectDrawSurface_AddRef stub called");
}


HRESULT WINAPI DirectDrawSurface_AddAttachedSurface(LPDIRECTDRAWSURFACE iface, LPDIRECTDRAWSURFACE lpDDSAttachedSurface) {
    kpanic("DirectDrawSurface_AddAttachedSurface stub called");
}

HRESULT WINAPI DirectDrawSurface_AddOverlayDirtyRect(LPDIRECTDRAWSURFACE iface, LPRECT lpRect) {
    kpanic("DirectDrawSurface_AddOverlayDirtyRect stub called");
}

HRESULT WINAPI DirectDrawSurface_BltBatch(LPDIRECTDRAWSURFACE iface, LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags) {
    kpanic("DirectDrawSurface_BltBatch stub called");
}

HRESULT WINAPI DirectDrawSurface_BltFast(LPDIRECTDRAWSURFACE iface, DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans) {
    kpanic("DirectDrawSurface_BltFast stub called");
}

HRESULT WINAPI DirectDrawSurface_DeleteAttachedSurface(LPDIRECTDRAWSURFACE iface, DWORD dwFlags, LPDIRECTDRAWSURFACE lpDDSAttachedSurface) {
    kpanic("DirectDrawSurface_DeleteAttachedSurface stub called");
}

HRESULT WINAPI DirectDrawSurface_EnumAttachedSurfaces(LPDIRECTDRAWSURFACE iface, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback) {
    kpanic("DirectDrawSurface_EnumAttachedSurfaces stub called");
}

HRESULT WINAPI DirectDrawSurface_EnumOverlayZOrders(LPDIRECTDRAWSURFACE iface, DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfnCallback) {
    kpanic("DirectDrawSurface_EnumOverlayZOrders stub called");
}

HRESULT WINAPI DirectDrawSurface_Flip(LPDIRECTDRAWSURFACE iface, LPDIRECTDRAWSURFACE lpDDSurfaceTargetOverride, DWORD dwFlags) {
    kpanic("DirectDrawSurface_Flip stub called");
}

HRESULT WINAPI DirectDrawSurface_GetAttachedSurface(LPDIRECTDRAWSURFACE iface, LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE *lplpDDAttachedSurface) {
    kpanic("DirectDrawSurface_GetAttachedSurface stub called");
}

HRESULT WINAPI DirectDrawSurface_GetBltStatus(LPDIRECTDRAWSURFACE iface, DWORD dwFlags) {
    kpanic("DirectDrawSurface_GetBltStatus stub called");
}

HRESULT WINAPI DirectDrawSurface_GetCaps(LPDIRECTDRAWSURFACE iface, LPDDSCAPS lpDDSCaps) {
    kpanic("DirectDrawSurface_GetCaps stub called");
}

HRESULT WINAPI DirectDrawSurface_GetClipper(LPDIRECTDRAWSURFACE iface, LPDIRECTDRAWCLIPPER *lplpDDClipper) {
    kpanic("DirectDrawSurface_GetClipper stub called");
}

HRESULT WINAPI DirectDrawSurface_GetColorKey(LPDIRECTDRAWSURFACE iface, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey) {
    GET_THIS;
    kpanicf("DirectDrawSurface_GetColorKey(%p, %d, %p); type = %d", iface, dwFlags, lpDDColorKey, this->type);
}

HRESULT WINAPI DirectDrawSurface_GetDC(LPDIRECTDRAWSURFACE iface, HDC *lphDC) {
    kpanic("DirectDrawSurface_GetDC stub called");
}

HRESULT WINAPI DirectDrawSurface_GetFlipStatus(LPDIRECTDRAWSURFACE iface, DWORD dwFlags) {
    kpanic("DirectDrawSurface_GetFlipStatus stub called");
}

HRESULT WINAPI DirectDrawSurface_GetOverlayPosition(LPDIRECTDRAWSURFACE iface, LPLONG lplX, LPLONG lplY) {
    kpanic("DirectDrawSurface_GetOverlayPosition stub called");
}

HRESULT WINAPI DirectDrawSurface_GetPalette(LPDIRECTDRAWSURFACE iface, LPDIRECTDRAWPALETTE *lplpDDPalette) {
    kpanic("DirectDrawSurface_GetPalette stub called");
}

HRESULT WINAPI DirectDrawSurface_Initialize(LPDIRECTDRAWSURFACE iface, LPDIRECTDRAW lpDD, LPDDSURFACEDESC lpDDSurfaceDesc) {
    kpanic("DirectDrawSurface_Initialize stub called");
}

HRESULT WINAPI DirectDrawSurface_IsLost(LPDIRECTDRAWSURFACE iface) {
    kpanic("DirectDrawSurface_IsLost stub called");
}

HRESULT WINAPI DirectDrawSurface_ReleaseDC(LPDIRECTDRAWSURFACE iface, HDC hDC) {
    kpanic("DirectDrawSurface_ReleaseDC stub called");
}

HRESULT WINAPI DirectDrawSurface_Restore(LPDIRECTDRAWSURFACE iface) {
    kpanic("DirectDrawSurface_Restore stub called");
}

HRESULT WINAPI DirectDrawSurface_SetClipper(LPDIRECTDRAWSURFACE iface, LPDIRECTDRAWCLIPPER lpDDClipper) {
    kpanic("DirectDrawSurface_SetClipper stub called");
}

HRESULT WINAPI DirectDrawSurface_SetOverlayPosition(LPDIRECTDRAWSURFACE iface, LONG lX, LONG lY) {
    kpanic("DirectDrawSurface_SetOverlayPosition stub called");
}

HRESULT WINAPI DirectDrawSurface_SetPalette(LPDIRECTDRAWSURFACE iface, LPDIRECTDRAWPALETTE lpDDPalette) {
    kpanic("DirectDrawSurface_SetPalette stub called");
}

HRESULT WINAPI DirectDrawSurface_UpdateOverlay(LPDIRECTDRAWSURFACE iface, LPRECT lpSrcRect, LPDIRECTDRAWSURFACE lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx) {
    kpanic("DirectDrawSurface_UpdateOverlay stub called");
}

HRESULT WINAPI DirectDrawSurface_UpdateOverlayDisplay(LPDIRECTDRAWSURFACE iface, DWORD dwFlags) {
    kpanic("DirectDrawSurface_UpdateOverlayDisplay stub called");
}

HRESULT WINAPI DirectDrawSurface_UpdateOverlayZOrder(LPDIRECTDRAWSURFACE iface, DWORD dwFlags, LPDIRECTDRAWSURFACE lpDDSReference) {
    kpanic("DirectDrawSurface_UpdateOverlayZOrder stub called");
}
