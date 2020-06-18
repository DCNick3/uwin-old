
#include <windows.h>
#include <ksvc.h>

DWORD WINAPI GetFileVersionInfoSizeA(LPCSTR lptstrFilename, LPDWORD lpdwHandle) {
    *lpdwHandle = 0;
    
    kprintf("GetFileVersionInfoSizeA(%s, %p) err-stub called", lptstrFilename, lpdwHandle);
    
    return 0;
}


