
#include <windows.h>

#include "uw_ddraw.h"

#include <ksvc.h>

IDirectDrawSurfaceVtbl uw_DirectDrawSurface_Vtable =
{
    DirectDrawSurface_QueryInterface,
    DirectDrawSurface_AddRef,
    DirectDrawSurface_Release,
    
    DirectDrawSurface_AddAttachedSurface,
    DirectDrawSurface_AddOverlayDirtyRect,
    DirectDrawSurface_Blt,
    DirectDrawSurface_BltBatch,
    DirectDrawSurface_BltFast,
    DirectDrawSurface_DeleteAttachedSurface,
    DirectDrawSurface_EnumAttachedSurfaces,
    DirectDrawSurface_EnumOverlayZOrders,
    DirectDrawSurface_Flip,
    DirectDrawSurface_GetAttachedSurface,
    DirectDrawSurface_GetBltStatus,
    DirectDrawSurface_GetCaps,
    DirectDrawSurface_GetClipper,
    DirectDrawSurface_GetColorKey,
    DirectDrawSurface_GetDC,
    DirectDrawSurface_GetFlipStatus,
    DirectDrawSurface_GetOverlayPosition,
    DirectDrawSurface_GetPalette,
    DirectDrawSurface_GetPixelFormat,
    DirectDrawSurface_GetSurfaceDesc,
    DirectDrawSurface_Initialize,
    DirectDrawSurface_IsLost,
    DirectDrawSurface_Lock,
    DirectDrawSurface_ReleaseDC,
    DirectDrawSurface_Restore,
    DirectDrawSurface_SetClipper,
    DirectDrawSurface_SetColorKey,
    DirectDrawSurface_SetOverlayPosition,
    DirectDrawSurface_SetPalette,
    DirectDrawSurface_Unlock,
    DirectDrawSurface_UpdateOverlay,
    DirectDrawSurface_UpdateOverlayDisplay,
    DirectDrawSurface_UpdateOverlayZOrder,
};


IDirectDrawVtbl uw_DirectDraw_Vtable =
{
    DirectDraw_QueryInterface,
    DirectDraw_AddRef,
    DirectDraw_Release,
    
    DirectDraw_Compact,
    DirectDraw_CreateClipper,
    DirectDraw_CreatePalette,
    DirectDraw_CreateSurface,
    DirectDraw_DuplicateSurface,
    DirectDraw_EnumDisplayModes,
    DirectDraw_EnumSurfaces,
    DirectDraw_FlipToGDISurface,
    DirectDraw_GetCaps,
    DirectDraw_GetDisplayMode,
    DirectDraw_GetFourCCCodes,
    DirectDraw_GetGDISurface,
    DirectDraw_GetMonitorFrequency,
    DirectDraw_GetScanLine,
    DirectDraw_GetVerticalBlankStatus,
    DirectDraw_Initialize,
    DirectDraw_RestoreDisplayMode,
    DirectDraw_SetCooperativeLevel,
    DirectDraw_SetDisplayMode,
    DirectDraw_WaitForVerticalBlank,
}; 
