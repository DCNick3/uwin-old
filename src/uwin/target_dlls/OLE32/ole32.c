
// hacky hacks around mingw headers
#define _OLE32_

#include <windows.h>
#include <ksvc.h>

HRESULT WINAPI CoInitialize(LPVOID pvReserved) {
    kprintf("CoInitialize called in thread %d", GetCurrentThreadId());
    return S_OK;
}

static void printfguid(GUID guid) {
    kprintf("{%08lX-%04hX-%04hX-%02hhX%02hhX-%02hhX%02hhX%02hhX%02hhX%02hhX%02hhX}", 
        guid.Data1, guid.Data2, guid.Data3, 
        guid.Data4[0], guid.Data4[1], guid.Data4[2], guid.Data4[3],
        guid.Data4[4], guid.Data4[5], guid.Data4[6], guid.Data4[7]);
}

HRESULT WINAPI CoCreateInstance(REFCLSID rclsid, LPUNKNOWN pUnkOuter, DWORD dwClsContext, REFIID riid, LPVOID *ppv) {
    kprintf("clsid:");
    printfguid(*rclsid);
    kprintf("iid:");
    printfguid(*riid);
    
    kprintf("CoCreateInstance(%p, %p, %x, %p, %p) err-stub called", rclsid, pUnkOuter, dwClsContext, riid, ppv);
    
    *ppv = NULL;
    
    return REGDB_E_CLASSNOTREG;
}
