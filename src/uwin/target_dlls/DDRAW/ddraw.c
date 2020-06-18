
#include <windows.h>
#include <ksvc.h>
#include <rpmalloc.h>

#include "uw_ddraw.h"


HRESULT WINAPI DirectDrawCreate(GUID *lpGUID, LPDIRECTDRAW *lplpDD, IUnknown *pUnkOuter) {
    
    uw_DirectDraw_t* ddraw = (uw_DirectDraw_t*)*lplpDD;
    
    if (ddraw != NULL || lpGUID != NULL || pUnkOuter != NULL)
        goto bad;
    
    ddraw = rpmalloc(sizeof(uw_DirectDraw_t));
    if (ddraw == NULL)
        kpanic("no memory for uw_DirectDraw_t");
    memset(ddraw, 0, sizeof(uw_DirectDraw_t));
    
    ddraw->lpVtbl = &uw_DirectDraw_Vtable;
    *lplpDD = (LPDIRECTDRAW)ddraw;
    
    return DD_OK;
    
bad:
    kpanicf("DirectDrawCreate(%p, %p, %p); lpDD = %p", lpGUID, lplpDD, pUnkOuter, *lplpDD);
}
