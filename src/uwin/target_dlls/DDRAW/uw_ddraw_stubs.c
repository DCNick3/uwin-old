
#include <windows.h>

#include "uw_ddraw.h"

#include <ksvc.h>

HRESULT WINAPI DirectDraw_QueryInterface(LPDIRECTDRAW iface, REFIID iid, LPVOID *ppObj) {
    kpanicf("DirectDraw_QueryInterface stub called");
}
ULONG WINAPI DirectDraw_AddRef(LPDIRECTDRAW iface)
{
    kpanicf("DirectDraw_AddRef stub called");
}

HRESULT WINAPI DirectDraw_Compact(LPDIRECTDRAW iface)
{
    kpanicf("DirectDraw_Compact stub called");
}
HRESULT WINAPI DirectDraw_CreateClipper(LPDIRECTDRAW iface,DWORD dwFlags,LPDIRECTDRAWCLIPPER *ppClipper,IUnknown *pUnkOuter)
{
    kpanicf("DirectDraw_CreateClipper stub called");
}
HRESULT WINAPI DirectDraw_CreatePalette(LPDIRECTDRAW iface, DWORD dwFlags,
                LPPALETTEENTRY pEntries,
                LPDIRECTDRAWPALETTE *ppPalette,
                IUnknown *pUnkOuter)
{
    kpanicf("DirectDraw_CreatePalette stub called");
}

HRESULT WINAPI DirectDraw_DuplicateSurface(LPDIRECTDRAW iface, LPDIRECTDRAWSURFACE src, LPDIRECTDRAWSURFACE *dst)
{
    kpanicf("DirectDraw_DuplicateSurface stub called");
}

HRESULT WINAPI DirectDraw_EnumDisplayModes(LPDIRECTDRAW iface, DWORD dwFlags,
                LPDDSURFACEDESC pDDSD, LPVOID context,
                LPDDENUMMODESCALLBACK cb)
{
    kpanicf("DirectDraw_EnumDisplayModes stub called");
}

HRESULT WINAPI DirectDraw_EnumSurfaces(LPDIRECTDRAW iface, DWORD dwFlags,
                LPDDSURFACEDESC pDDSD, LPVOID context,
                LPDDENUMSURFACESCALLBACK cb)
{
    kpanicf("DirectDraw_EnumSurfaces stub called");
}

HRESULT WINAPI DirectDraw_FlipToGDISurface(LPDIRECTDRAW iface)
{
    kpanicf("DirectDraw_FlipToGDISurface stub called");
}

HRESULT WINAPI DirectDraw_GetCaps(LPDIRECTDRAW iface, LPDDCAPS pDDC1, LPDDCAPS pDDC2)
{
    kpanicf("DirectDraw_GetCaps stub called");
}

HRESULT WINAPI DirectDraw_GetDisplayMode(LPDIRECTDRAW iface, LPDDSURFACEDESC pDDSD)
{
    kpanicf("DirectDraw_GetDisplayMode stub called");
}

HRESULT WINAPI DirectDraw_GetFourCCCodes(LPDIRECTDRAW iface, LPDWORD pNumCodes,
                LPDWORD pCodes)
{
    kpanicf("DirectDraw_GetFourCCCodes stub called");
}

HRESULT WINAPI DirectDraw_GetGDISurface(LPDIRECTDRAW iface, LPDIRECTDRAWSURFACE *lplpGDIDDSSurface)
{
    kpanicf("DirectDraw_GetGDISurface stub called");
}

HRESULT WINAPI DirectDraw_GetMonitorFrequency(LPDIRECTDRAW iface, LPDWORD pdwFreq)
{
    kpanicf("DirectDraw_GetMonitorFrequency stub called");
}

HRESULT WINAPI DirectDraw_GetScanLine(LPDIRECTDRAW iface, LPDWORD lpdwScanLine)
{
    kpanicf("DirectDraw_GetScanLine stub called");
}

HRESULT WINAPI DirectDraw_GetVerticalBlankStatus(LPDIRECTDRAW iface, LPBOOL lpbIsInVB)
{
    kpanicf("DirectDraw_GetVerticalBlankStatus stub called");
}

HRESULT WINAPI DirectDraw_Initialize(LPDIRECTDRAW iface, LPGUID pGUID)
{
    kpanicf("DirectDraw_Initialize stub called");
}

HRESULT WINAPI DirectDraw_WaitForVerticalBlank(LPDIRECTDRAW iface, DWORD dwFlags, HANDLE hEvent)
{
    kpanicf("DirectDraw_WaitForVerticalBlank stub called");
}
