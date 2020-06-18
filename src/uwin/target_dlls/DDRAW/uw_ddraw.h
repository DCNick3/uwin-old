
#ifndef UW_DDRAW_H
#define UW_DDRAW_H

#include <windows.h>
#include <ddraw.h>
#include <ksvc.h>

#define HWND_DUMMY_GAME    ((HWND)1) 

#define UW_SURFACE_SCREEN 1
#define UW_SURFACE_BUFFER 2

typedef struct {
    IDirectDrawVtbl* lpVtbl;
    int screen_created;
} uw_DirectDraw_t;

typedef struct {
    IDirectDrawSurfaceVtbl* lpVtbl;
    int type;
    khandle_t handle;
    int has_bltsrc_colorkey;
    int locked;
    uint32_t colorkey;
} uw_DirectDrawSurface_t;

extern IDirectDrawSurfaceVtbl   uw_DirectDrawSurface_Vtable;
extern IDirectDrawVtbl          uw_DirectDraw_Vtable;

HRESULT WINAPI DirectDraw_QueryInterface(LPDIRECTDRAW iface, REFIID riid, void** ppvObject);
ULONG WINAPI DirectDraw_AddRef(LPDIRECTDRAW iface);
ULONG WINAPI DirectDraw_Release(LPDIRECTDRAW iface);
HRESULT WINAPI DirectDraw_Compact(LPDIRECTDRAW iface);
HRESULT WINAPI DirectDraw_CreateClipper(LPDIRECTDRAW iface, DWORD dwFlags, LPDIRECTDRAWCLIPPER *lplpDDClipper, IUnknown *pUnkOuter);
HRESULT WINAPI DirectDraw_CreatePalette(LPDIRECTDRAW iface, DWORD dwFlags, LPPALETTEENTRY lpColorTable, LPDIRECTDRAWPALETTE *lplpDDPalette, IUnknown *pUnkOuter);
HRESULT WINAPI DirectDraw_CreateSurface(LPDIRECTDRAW iface, LPDDSURFACEDESC lpDDSurfaceDesc, LPDIRECTDRAWSURFACE *lplpDDSurface, IUnknown *pUnkOuter);
HRESULT WINAPI DirectDraw_DuplicateSurface(LPDIRECTDRAW iface, LPDIRECTDRAWSURFACE lpDDSurface, LPDIRECTDRAWSURFACE *lplpDupDDSurface);
HRESULT WINAPI DirectDraw_EnumDisplayModes(LPDIRECTDRAW iface, DWORD dwFlags, LPDDSURFACEDESC lpDDSurfaceDesc, LPVOID lpContext, LPDDENUMMODESCALLBACK lpEnumModesCallback);
HRESULT WINAPI DirectDraw_EnumSurfaces(LPDIRECTDRAW iface, DWORD dwFlags, LPDDSURFACEDESC lpDDSD, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback);
HRESULT WINAPI DirectDraw_FlipToGDISurface(LPDIRECTDRAW iface);
HRESULT WINAPI DirectDraw_GetCaps(LPDIRECTDRAW iface, LPDDCAPS lpDDDriverCaps, LPDDCAPS lpDDHELCaps);
HRESULT WINAPI DirectDraw_GetDisplayMode(LPDIRECTDRAW iface, LPDDSURFACEDESC lpDDSurfaceDesc);
HRESULT WINAPI DirectDraw_GetFourCCCodes(LPDIRECTDRAW iface, LPDWORD lpNumCodes, LPDWORD lpCodes);
HRESULT WINAPI DirectDraw_GetGDISurface(LPDIRECTDRAW iface, LPDIRECTDRAWSURFACE *lplpGDIDDSurface);
HRESULT WINAPI DirectDraw_GetMonitorFrequency(LPDIRECTDRAW iface, LPDWORD lpdwFrequency);
HRESULT WINAPI DirectDraw_GetScanLine(LPDIRECTDRAW iface, LPDWORD lpdwScanLine);
HRESULT WINAPI DirectDraw_GetVerticalBlankStatus(LPDIRECTDRAW iface, BOOL *lpbIsInVB);
HRESULT WINAPI DirectDraw_Initialize(LPDIRECTDRAW iface, GUID *lpGUID);
HRESULT WINAPI DirectDraw_RestoreDisplayMode(LPDIRECTDRAW iface);
HRESULT WINAPI DirectDraw_SetCooperativeLevel(LPDIRECTDRAW iface, HWND hWnd, DWORD dwFlags);
HRESULT WINAPI DirectDraw_SetDisplayMode(LPDIRECTDRAW iface, DWORD dwWidth, DWORD dwHeight, DWORD dwBPP);
HRESULT WINAPI DirectDraw_WaitForVerticalBlank(LPDIRECTDRAW iface, DWORD dwFlags, HANDLE hEvent);

HRESULT WINAPI DirectDrawSurface_QueryInterface(LPDIRECTDRAWSURFACE iface, REFIID riid, void** ppvObject);
ULONG WINAPI DirectDrawSurface_AddRef(LPDIRECTDRAWSURFACE iface);
ULONG WINAPI DirectDrawSurface_Release(LPDIRECTDRAWSURFACE iface);
HRESULT WINAPI DirectDrawSurface_AddAttachedSurface(LPDIRECTDRAWSURFACE iface, LPDIRECTDRAWSURFACE lpDDSAttachedSurface);
HRESULT WINAPI DirectDrawSurface_AddOverlayDirtyRect(LPDIRECTDRAWSURFACE iface, LPRECT lpRect);
HRESULT WINAPI DirectDrawSurface_Blt(LPDIRECTDRAWSURFACE iface, LPRECT lpDestRect, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwFlags, LPDDBLTFX lpDDBltFx);
HRESULT WINAPI DirectDrawSurface_BltBatch(LPDIRECTDRAWSURFACE iface, LPDDBLTBATCH lpDDBltBatch, DWORD dwCount, DWORD dwFlags);
HRESULT WINAPI DirectDrawSurface_BltFast(LPDIRECTDRAWSURFACE iface, DWORD dwX, DWORD dwY, LPDIRECTDRAWSURFACE lpDDSrcSurface, LPRECT lpSrcRect, DWORD dwTrans);
HRESULT WINAPI DirectDrawSurface_DeleteAttachedSurface(LPDIRECTDRAWSURFACE iface, DWORD dwFlags, LPDIRECTDRAWSURFACE lpDDSAttachedSurface);
HRESULT WINAPI DirectDrawSurface_EnumAttachedSurfaces(LPDIRECTDRAWSURFACE iface, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpEnumSurfacesCallback);
HRESULT WINAPI DirectDrawSurface_EnumOverlayZOrders(LPDIRECTDRAWSURFACE iface, DWORD dwFlags, LPVOID lpContext, LPDDENUMSURFACESCALLBACK lpfnCallback);
HRESULT WINAPI DirectDrawSurface_Flip(LPDIRECTDRAWSURFACE iface, LPDIRECTDRAWSURFACE lpDDSurfaceTargetOverride, DWORD dwFlags);
HRESULT WINAPI DirectDrawSurface_GetAttachedSurface(LPDIRECTDRAWSURFACE iface, LPDDSCAPS lpDDSCaps, LPDIRECTDRAWSURFACE *lplpDDAttachedSurface);
HRESULT WINAPI DirectDrawSurface_GetBltStatus(LPDIRECTDRAWSURFACE iface, DWORD dwFlags);
HRESULT WINAPI DirectDrawSurface_GetCaps(LPDIRECTDRAWSURFACE iface, LPDDSCAPS lpDDSCaps);
HRESULT WINAPI DirectDrawSurface_GetClipper(LPDIRECTDRAWSURFACE iface, LPDIRECTDRAWCLIPPER *lplpDDClipper);
HRESULT WINAPI DirectDrawSurface_GetColorKey(LPDIRECTDRAWSURFACE iface, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
HRESULT WINAPI DirectDrawSurface_GetDC(LPDIRECTDRAWSURFACE iface, HDC *lphDC);
HRESULT WINAPI DirectDrawSurface_GetFlipStatus(LPDIRECTDRAWSURFACE iface, DWORD dwFlags);
HRESULT WINAPI DirectDrawSurface_GetOverlayPosition(LPDIRECTDRAWSURFACE iface, LPLONG lplX, LPLONG lplY);
HRESULT WINAPI DirectDrawSurface_GetPalette(LPDIRECTDRAWSURFACE iface, LPDIRECTDRAWPALETTE *lplpDDPalette);
HRESULT WINAPI DirectDrawSurface_GetPixelFormat(LPDIRECTDRAWSURFACE iface, LPDDPIXELFORMAT lpDDPixelFormat);
HRESULT WINAPI DirectDrawSurface_GetSurfaceDesc(LPDIRECTDRAWSURFACE iface, LPDDSURFACEDESC lpDDSurfaceDesc);
HRESULT WINAPI DirectDrawSurface_Initialize(LPDIRECTDRAWSURFACE iface, LPDIRECTDRAW lpDD, LPDDSURFACEDESC lpDDSurfaceDesc);
HRESULT WINAPI DirectDrawSurface_IsLost(LPDIRECTDRAWSURFACE iface);
HRESULT WINAPI DirectDrawSurface_Lock(LPDIRECTDRAWSURFACE iface, LPRECT lpDestRect, LPDDSURFACEDESC lpDDSurfaceDesc, DWORD dwFlags, HANDLE hEvent);
HRESULT WINAPI DirectDrawSurface_ReleaseDC(LPDIRECTDRAWSURFACE iface, HDC hDC);
HRESULT WINAPI DirectDrawSurface_Restore(LPDIRECTDRAWSURFACE iface);
HRESULT WINAPI DirectDrawSurface_SetClipper(LPDIRECTDRAWSURFACE iface, LPDIRECTDRAWCLIPPER lpDDClipper);
HRESULT WINAPI DirectDrawSurface_SetColorKey(LPDIRECTDRAWSURFACE iface, DWORD dwFlags, LPDDCOLORKEY lpDDColorKey);
HRESULT WINAPI DirectDrawSurface_SetOverlayPosition(LPDIRECTDRAWSURFACE iface, LONG lX, LONG lY);
HRESULT WINAPI DirectDrawSurface_SetPalette(LPDIRECTDRAWSURFACE iface, LPDIRECTDRAWPALETTE lpDDPalette);
HRESULT WINAPI DirectDrawSurface_Unlock(LPDIRECTDRAWSURFACE iface, LPVOID lpSurfaceData);
HRESULT WINAPI DirectDrawSurface_UpdateOverlay(LPDIRECTDRAWSURFACE iface, LPRECT lpSrcRect, LPDIRECTDRAWSURFACE lpDDDestSurface, LPRECT lpDestRect, DWORD dwFlags, LPDDOVERLAYFX lpDDOverlayFx);
HRESULT WINAPI DirectDrawSurface_UpdateOverlayDisplay(LPDIRECTDRAWSURFACE iface, DWORD dwFlags);
HRESULT WINAPI DirectDrawSurface_UpdateOverlayZOrder(LPDIRECTDRAWSURFACE iface, DWORD dwFlags, LPDIRECTDRAWSURFACE lpDDSReference);

#endif
