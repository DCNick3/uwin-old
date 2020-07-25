/*
 *  kernel32.dll implementation
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

#define __MINGW_EXCPT_DEFINE_PSDK

// hacky hacks around mingw headers
#define WINBASEAPI __declspec(dllexport)

#define _NTSYSTEM_

#include <windows.h>
#include <ksvc.h>
#include <rpmalloc.h>
#include <printf.h>

//#undef WINAPI
//#define WINAPI __stdcall

/*#define DUMMY_STDIN_HANDLE  ((HANDLE)0x111)
#define DUMMY_STDOUT_HANDLE ((HANDLE)0x222)
#define DUMMY_STDERR_HANDLE ((HANDLE)0x333)*/

static HANDLE dummy_stdin_handle;
static HANDLE dummy_stdout_handle;
static HANDLE dummy_stderr_handle;
static HANDLE dummy_current_process_handle;
//static HANDLE dummy_thread_handles[KTHREAD_ID_MAX+1];

static DWORD next_tls_index = 2;

static LPTOP_LEVEL_EXCEPTION_FILTER top_level_exception_filter;
static uint64_t start_time;

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
    kprintf("kernel32 init");
    if (fdwReason == DLL_PROCESS_ATTACH) {
        dummy_stdin_handle = (HANDLE)kht_dummy_new(KHT_DUMMY_CONIO, 0);
        dummy_stdout_handle = (HANDLE)kht_dummy_new(KHT_DUMMY_CONIO, 0);
        dummy_stderr_handle = (HANDLE)kht_dummy_new(KHT_DUMMY_CONIO, 0);
        dummy_current_process_handle = (HANDLE)kht_dummy_new(KHT_DUMMY_PROCESS, 0);
        
        /*for (int i = 0; i <= KTHREAD_ID_MAX; i++) {
            dummy_thread_handles[i] = (HANDLE)kht_dummy_new(KHT_DUMMY_THREAD, i);
        }*/
        
        start_time = kget_monotonic_time();
    }
    return TRUE;
}


DWORD WINAPI GetVersion() {
    return 0x0ece0205;
}

DWORD WINAPI GetLastError() {
    return (DWORD)get_teb_value(TEB_LAST_ERROR);
}

VOID WINAPI SetLastError(DWORD dwErrCode) {
    return set_teb_value(TEB_LAST_ERROR, (void*)dwErrCode);
}

HMODULE WINAPI GetModuleHandleA(LPCSTR lpModuleName) {
    return (HMODULE)win32_get_module_handle(lpModuleName);
}

DWORD WINAPI GetModuleFileNameA(HMODULE hModule, LPSTR lpFilename, DWORD nSize) {
    return win32_get_module_file_name((uint32_t)hModule, lpFilename, nSize);
}

FARPROC WINAPI GetProcAddress(HMODULE hModule, LPCSTR lpProcName) {
    void* r = win32_get_proc_address((uint32_t)hModule, lpProcName);
    kprintf("GetProcAddress(%p, %s) -> %p", hModule, lpProcName, r);
    return r;
}

HANDLE WINAPI HeapCreate(DWORD flOptions, SIZE_T dwInitialSize, SIZE_T dwMaximumSize) {
    if (flOptions & HEAP_GENERATE_EXCEPTIONS)
        kpanic("HEAP_GENERATE_EXCEPTIONS is unsupported");
    return rpmalloc_heap_acquire();
}

BOOL WINAPI HeapDestroy(HANDLE hHeap) {
    kprintf("HeapDestroy(%p)", hHeap);
    rpmalloc_heap_t* heap = (rpmalloc_heap_t*)hHeap;
    rpmalloc_heap_free_all(heap);
    rpmalloc_heap_release(heap);
}


LPVOID WINAPI HeapAlloc(HANDLE hHeap, DWORD dwFlags, SIZE_T dwBytes) {
    //kprintf("HeapAlloc(%p, %x, %x)", hHeap, dwFlags, dwBytes);
    rpmalloc_heap_t* heap = (rpmalloc_heap_t*)hHeap;
    if (dwFlags & HEAP_NO_SERIALIZE)
        kpanicf("HeapAlloc(%p, %x, %x)", hHeap, dwFlags, dwBytes);
    void *res;
    if (dwFlags & HEAP_ZERO_MEMORY)
        res = rpmalloc_heap_calloc(heap, 1, dwBytes);
    else
        res = rpmalloc_heap_alloc(heap, dwBytes);
    if (res == NULL) {
        if (dwFlags & HEAP_GENERATE_EXCEPTIONS)
            kpanicf("HeapAlloc(%p, %x, %x) is out of memory", hHeap, dwFlags, dwBytes);
        return NULL;
    }
    
    return res;
}

BOOL WINAPI HeapFree(HANDLE hHeap, DWORD dwFlags, LPVOID lpMem) {
    //kprintf("HeapFree(%p, %x, %p)", hHeap, dwFlags, lpMem);
    rpmalloc_heap_t* heap = (rpmalloc_heap_t*)hHeap;
    rpmalloc_heap_free(heap, lpMem);
    return TRUE;
}

SIZE_T WINAPI HeapSize(HANDLE hHeap, DWORD dwFlags, LPCVOID lpMem) {
    return rpmalloc_usable_size((void*)lpMem);
}

static uint32_t conv_prot(DWORD flProt) {
    switch (flProt) {
        case PAGE_EXECUTE:
        case PAGE_EXECUTE_READ:
        case PAGE_READONLY:
            return PROT_R;
        case PAGE_EXECUTE_READWRITE:
        case PAGE_READWRITE:
            return PROT_RW;
        case PAGE_NOACCESS:
            return PROT_N;
        default:
            kpanicf("Unknown flProt: %x", flProt);
    }
}

LPVOID WINAPI VirtualAlloc(LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect) {
    kprintf("VirtualAlloc(%x, %x, %x, %x)", lpAddress, dwSize, flAllocationType, flProtect);
    LPVOID res = NULL;
    if (flAllocationType & MEM_RESERVE) {
        if (lpAddress != NULL) {
            res = kmap_memory_fixed(lpAddress, dwSize, PROT_N) ? NULL : lpAddress;
        } else {
            res = kmap_memory_dynamic(dwSize, PROT_N);
            res = res == (void*)-1 ? NULL : res;
        }
    } else if (flAllocationType & MEM_COMMIT) {
        if (lpAddress != NULL) {
            res = kmap_memory_fixed(lpAddress, dwSize, conv_prot(flProtect)) ? NULL : lpAddress;
        } else {
            res = kmap_memory_dynamic(dwSize, conv_prot(flProtect));
            res = res == (void*)-1 ? NULL : res;
        }
    } else {
        kpanicf("VirtualAlloc(%x, %x, %x, %x)", lpAddress, dwSize, flAllocationType, flProtect);
    }
    return res;
}

BOOL WINAPI VirtualFree(LPVOID lpAddress, SIZE_T dwSize, DWORD dwFreeType) {
    kprintf("VirtualFree(%x, %u, %x) no-op stub called", lpAddress, dwSize, dwFreeType);
}

LPSTR WINAPI GetCommandLineA() {
    kprintf("GetCommandLineA()");
    return (LPSTR)get_teb_value(TEB_CMDLINE);
}

static char environment_block[] = "\0\0";

LPCH WINAPI GetEnvironmentStrings() {
    return environment_block;
}

LPWCH WINAPI GetEnvironmentStringsW() {
    kprintf("GetEnvironmentStringsW err-stub called");
    return NULL;
}

BOOL WINAPI FreeEnvironmentStringsA(LPCH penv) {
    kprintf("FreeEnvironmentStringsA no-op stub called");
}

BOOL WINAPI FreeEnvironmentStringsW(LPWCH penv) {
    kpanicf("FreeEnvironmentStringsW");
}

VOID WINAPI GetStartupInfoA(LPSTARTUPINFOA lpStartupInfo) {
    kprintf("GetStartupInfoA(%p)");
    memset(lpStartupInfo, 0, sizeof(STARTUPINFOA));
    lpStartupInfo->cb = sizeof(STARTUPINFOA);
}

HANDLE WINAPI GetStdHandle(DWORD nStdHandle) {
    kprintf("GetStdHandle(%x)", nStdHandle);
    switch (nStdHandle) {
        case  STD_INPUT_HANDLE:     return dummy_stdin_handle;
        case STD_OUTPUT_HANDLE:     return dummy_stdout_handle;
        case  STD_ERROR_HANDLE:     return dummy_stderr_handle;
        default: kpanicf("GetStdHandle(%d)", nStdHandle);
    }
}

DWORD WINAPI GetFileType(HANDLE hFile) {
    kprintf ("GetFileType(%p)", hFile);
    if (hFile == dummy_stdin_handle || hFile == dummy_stdout_handle || hFile == dummy_stderr_handle)
        return FILE_TYPE_CHAR;
    uint32_t type = kht_gettype((khandle_t)hFile);
    if (type == KHT_FILE)
        return FILE_TYPE_DISK;
    kpanicf("GetFileType(%p); type = %d", hFile, type);
}

UINT WINAPI SetHandleCount(UINT uNumber) {
    kprintf("SetHandleCount(%u)", uNumber);
}

#define CP_US 437

UINT WINAPI GetACP() {
    kprintf("GetACP()");
    return CP_US;
}

BOOL WINAPI GetCPInfo(UINT CodePage, LPCPINFO lpCPInfo) {
    kprintf("GetCPInfo(%x, %p)", CodePage, lpCPInfo);
    if (CodePage != CP_US)
        kpanicf("GetCPInfo(%x, %p)", CodePage, lpCPInfo);
    lpCPInfo->MaxCharSize = 1;
    lpCPInfo->DefaultChar[0] = '?';
    lpCPInfo->DefaultChar[1] = '\0';
    for (int i = 0; i < sizeof(lpCPInfo->LeadByte) / sizeof(BYTE); i++) {
        lpCPInfo->LeadByte[i] = 0;
    }
    return TRUE;
}

static char windows_directory[] = "C:\\Windows";

UINT WINAPI GetWindowsDirectoryA(LPSTR lpBuffer, UINT uSize) {
    kprintf("GetWindowsDirectoryA(%p, %x)", lpBuffer, uSize);
    if (uSize < sizeof(windows_directory))
        kpanicf("GetWindowsDirectoryA(%p, %d)", lpBuffer, uSize);
    memcpy(lpBuffer, windows_directory, sizeof(windows_directory));
    return sizeof(windows_directory);
}

static char system_directory[] = "C:\\Windows\\System32";

UINT WINAPI GetSystemDirectoryA(LPSTR lpBuffer, UINT uSize) {
    kprintf("GetSystemDirectoryA(%p, %x)", lpBuffer, uSize);
    if (uSize < sizeof(system_directory))
        kpanicf("GetSystemDirectoryA(%p, %d)", lpBuffer, uSize);
    memcpy(lpBuffer, system_directory, sizeof(system_directory));
    return sizeof(system_directory);
}

static char service_pack_name[] = "Service Pack 2";

BOOL WINAPI GetVersionExA(LPOSVERSIONINFOA lpVersionInformation) {
    kprintf("GetVersionExA(%p)", lpVersionInformation);
    if (lpVersionInformation->dwOSVersionInfoSize != sizeof(OSVERSIONINFOA))
        kpanicf("GetVersionExA(%p); size = %d", lpVersionInformation, lpVersionInformation->dwOSVersionInfoSize);
    lpVersionInformation->dwMajorVersion = 5;
    lpVersionInformation->dwMinorVersion = 5;
    lpVersionInformation->dwBuildNumber = 3790;
    lpVersionInformation->dwPlatformId = 2;
    memcpy(lpVersionInformation->szCSDVersion, service_pack_name, sizeof(service_pack_name));
    return TRUE;
}

BOOL WINAPI QueryPerformanceFrequency(LARGE_INTEGER *lpFrequency) {
    kprintf("QueryPerformanceFrequency(%p)", lpFrequency);
    lpFrequency->QuadPart = 10000000;
    return TRUE;
}

BOOL WINAPI QueryPerformanceCounter(LARGE_INTEGER *lpPerformanceCount) {
    lpPerformanceCount->QuadPart = kget_monotonic_time() * 10;
    kprintf("QueryPerformanceCounter -> %lu", (unsigned long)lpPerformanceCount->QuadPart);
    return TRUE;
}

BOOL WINAPI DisableThreadLibraryCalls(HMODULE lpLibmodule) {
    kprintf("DisableThreadLibraryCalls(%p)", lpLibmodule);
    return TRUE;
}

BOOL WINAPI GetStringTypeW(DWORD dwInfoType, LPCWCH lpSrcStr, int cchSrc, LPWORD lpCharType) {
    kprintf("GetStringTypeW err-stub called");
    return FALSE;
}

BOOL WINAPI GetStringTypeA(LCID Locale, DWORD dwInfoType, LPCSTR lpSrcStr, int cchSrc, LPWORD lpCharType) {
    kprintf("GetStringTypeA err-stub called");
    return FALSE;
}

INT WINAPI LCMapStringW(LCID Locale, DWORD dwMapFlags, LPCWSTR lpSrcStr, INT cchSrc, LPWSTR lpDestStr, INT cchDest) {
    kprintf("LCMapStringW err-stub called");
    return 0;
}

INT WINAPI LCMapStringA(LCID Locale, DWORD dwMapFlags, LPCSTR lpSrcStr, INT cchSrc, LPSTR lpDestStr, INT cchDest) {
    kprintf("LCMapStringA err-stub called");
    return 0;
}

// Thanks, ReactOS

VOID WINAPI InitializeCriticalSection(LPCRITICAL_SECTION lpCriticalSection) {
    kprintf("InitializeCriticalSection(%p)", lpCriticalSection);
    memset(lpCriticalSection, 0, sizeof(CRITICAL_SECTION));
    lpCriticalSection->LockSemaphore = (HANDLE)ksem_alloc(0);
    lpCriticalSection->LockCount = -1;
    lpCriticalSection->OwningThread = 0;
}

VOID WINAPI DeleteCriticalSection(LPCRITICAL_SECTION lpCriticalSection) {
    kprintf("DeleteCriticalSection(%p)", lpCriticalSection);
    kht_delref((khandle_t)lpCriticalSection->LockSemaphore, NULL, NULL);
}

VOID WINAPI EnterCriticalSection(LPCRITICAL_SECTION CriticalSection) {
    //kprintf("EnterCriticalSection(%p)", CriticalSection);
    HANDLE Thread = GetCurrentThread();
 
    /* Try to lock it */
    if (InterlockedIncrement(&CriticalSection->LockCount) != 0)
    {
        /* We've failed to lock it! Does this thread actually own it? */
        if (Thread == CriticalSection->OwningThread)
        {
            /*
            * You own it, so you'll get it when you're done with it! No need to
            * use the interlocked functions as only the thread who already owns
            * the lock can modify this data.
            */
            CriticalSection->RecursionCount++;
            return;
        }

        /* NOTE - CriticalSection->OwningThread can be NULL here because changing
                this information is not serialized. This happens when thread a
                acquires the lock (LockCount == 0) and thread b tries to
                acquire it as well (LockCount == 1) but thread a hasn't had a
                chance to set the OwningThread! So it's not an error when
                OwningThread is NULL here! */

        /* We don't own it, so we must wait for it */
        ksem_wait((khandle_t)CriticalSection->LockSemaphore, KSEM_TMO_INF);
    }

    /*
    * Lock successful. Changing this information has not to be serialized
    * because only one thread at a time can actually change it (the one who
    * acquired the lock)!
    */
    CriticalSection->OwningThread = Thread;
    CriticalSection->RecursionCount = 1;
}

VOID WINAPI LeaveCriticalSection(LPCRITICAL_SECTION CriticalSection) {
    // kprintf("LeaveCriticalSection(%p)", CriticalSection);
#if 0
    HANDLE Thread = GetCurrentThread();

    /*
    * In win this case isn't checked. However it's a valid check so it should
    * only be performed in debug builds!
    */
    if (Thread != CriticalSection->OwningThread)
    {
        kpanic("Releasing critical section not owned! Owner = %p, Me = %p", CriticalSection->OwningThread, Thread);
    }
#endif

    /*
    * Decrease the Recursion Count. No need to do this atomically because only
    * the thread who holds the lock can call this function (unless the program
    * is totally screwed...
    */
    if (--CriticalSection->RecursionCount)
    {
        /* Someone still owns us, but we are free. This needs to be done atomically. */
        InterlockedDecrement(&CriticalSection->LockCount);
    }
    else
    {
        /*
        * Nobody owns us anymore. No need to do this atomically.
        * See comment above.
        */
        CriticalSection->OwningThread = 0;

        /* Was someone wanting us? This needs to be done atomically. */
        if (-1 != InterlockedDecrement(&CriticalSection->LockCount))
        {
            /* Let him have us */
            ksem_post((khandle_t)CriticalSection->LockSemaphore);
        }
    }
}

DWORD WINAPI TlsAlloc() {
    kprintf("TlsAlloc()");
    return __atomic_fetch_add(&next_tls_index, 1, __ATOMIC_SEQ_CST);
}

LPVOID WINAPI TlsGetValue(DWORD dwIndex) {
    //kprintf("TlsGetValue(%x)", dwIndex);
    return get_tls_value(dwIndex * 4);
}

BOOL WINAPI TlsSetValue(DWORD dwIndex, LPVOID value) {
    kprintf("TlsSetValue(%x, %p)", dwIndex, value);
    set_tls_value(dwIndex * 4, value);
    return TRUE;
}

DWORD WINAPI GetCurrentThreadId() {
    //kprintf("GetCurrentThreadId()");
    return (DWORD)get_teb_value(TEB_TID);
}

LPTOP_LEVEL_EXCEPTION_FILTER WINAPI SetUnhandledExceptionFilter(LPTOP_LEVEL_EXCEPTION_FILTER lpTopLevelExceptionFilter) {
    kprintf("SetUnhandledExceptionFilter(%p)", lpTopLevelExceptionFilter);
    return __atomic_exchange_n(&top_level_exception_filter, lpTopLevelExceptionFilter, __ATOMIC_SEQ_CST);
}

// SEH. Uses stuff in TEB
// see http://web.archive.org/web/20180115191634/http://www.microsoft.com:80/msj/0197/exception/exception.aspx
// see https://github.com/wine-mirror/wine/blob/ba9f3dc198dfc81bb40159077b73b797006bb73c/dlls/ntdll/signal_i386.c

#define EH_NONCONTINUABLE   0x01
#define EH_UNWINDING        0x02
#define EH_EXIT_UNWIND      0x04
#define EH_STACK_INVALID    0x08
#define EH_NESTED_CALL      0x10
#define EH_TARGET_UNWIND    0x20
#define EH_COLLIDED_UNWIND  0x40

typedef EXCEPTION_DISPOSITION WINAPI (*currect_exc_handler_t)(struct _EXCEPTION_RECORD*, void*, struct _CONTEXT*, void*);

VOID WINAPI RaiseException(DWORD code, DWORD flags, DWORD nbargs, const ULONG_PTR *args) {
    kprintf("RaiseException(%x, %x, %d, %p)", code, flags, nbargs, args);

    kprintf("ret[0] = %p", __builtin_return_address(0));
    kprintf("ret[1] = %p", __builtin_return_address(1));
    kprintf("ret[2] = %p", __builtin_return_address(2));
    kprintf("ret[3] = %p", __builtin_return_address(3));

    // build an EXCEPTION_RECORD
    EXCEPTION_RECORD record;
    record.ExceptionCode    = code;
    record.ExceptionFlags   = flags & EH_NONCONTINUABLE;
    record.ExceptionRecord  = NULL;
    record.ExceptionAddress = RaiseException;

    // build a dummy CONTEXT
    CONTEXT context;
    memset(&context, 0, sizeof(context));
    
    if (nbargs && args)
    {
        if (nbargs > EXCEPTION_MAXIMUM_PARAMETERS) nbargs = EXCEPTION_MAXIMUM_PARAMETERS;
        record.NumberParameters = nbargs;
        memcpy( record.ExceptionInformation, args, nbargs * sizeof(*args) );
    }
    else record.NumberParameters = 0;
    
    // now traverse the SEH handler list
    
    EXCEPTION_REGISTRATION* frame = get_teb_value(TEB_SEH_FRAME);
    void *dispatch = NULL;
    
    kprintf("exception raised, calling handlers");
    
    while (frame != (EXCEPTION_REGISTRATION*)~0) {
        kprintf("calling handler: %p %p", frame, frame->handler);
        
        DWORD res = (frame->handler)(&record, frame, &context, &dispatch);
        
        if (res == ExceptionContinueSearch) {
            kprintf("handler returned ExceptionContinueSearch");
        } else if (res == ExceptionContinueExecution) {
            kprintf("handler returned ExceptionContinueExecution");
            return;
        } else {
            kpanicf("handler returned %x", res);
        }
        
        // we want to pop frame
        EXCEPTION_REGISTRATION* prevframe = frame->prev;
        set_teb_value(0, prevframe);
        frame = prevframe;
    }
    
    kpanic("F");
}

PVOID WINAPI RtlUnwind_real(PVOID pEndFrame, PVOID targetIp, PEXCEPTION_RECORD pRecord, PVOID retval) {
    kprintf("RtlUnwind_real(%p, %p, %p, %p)", pEndFrame, targetIp, pRecord, retval);
    
    pRecord->ExceptionFlags |= EH_UNWINDING | (pEndFrame ? 0 : EH_EXIT_UNWIND);
    
    EXCEPTION_REGISTRATION* frame = get_teb_value(TEB_SEH_FRAME);
    
    // build a dummy CONTEXT
    CONTEXT context;
    memset(&context, 0, sizeof(context));
    
    void *dispatch = NULL;
    
    kprintf("unwind requested, calling handlers");
    
    while (frame != (EXCEPTION_REGISTRATION*)~0 && (frame != pEndFrame)) {
        if (pEndFrame && (frame > (EXCEPTION_REGISTRATION*)pEndFrame))
            kpanicf("invalid unwind target: %p, %p, %p", pRecord, frame, pEndFrame);
        
        kprintf("calling handler: %p %p", frame, frame->handler);
        
        DWORD res = (frame->handler)(pRecord, frame, &context, &dispatch);
        
        if (res == ExceptionContinueSearch) {
            kprintf("handler returned ExceptionContinueSearch");
        } else {
            kpanicf("handler returned %x", res);
        }
        
        // we want to pop frame
        EXCEPTION_REGISTRATION* prevframe = frame->prev;
        set_teb_value(0, prevframe);
        frame = prevframe;
    }
    
    //kprintf("CONTEXT: ");
    
    kprintf("unwinding done. returning");
    //mu_frame[1] = targetIp;
    //kpanic("zzz");
    if (__builtin_return_address(0) != targetIp)
        kpanicf("unsupported targetIp. have %x, want %x", targetIp, __builtin_return_address(0));
    
    return retval; // HaCk
}

//NTSYSAPI VOID WINAPI __attribute__((naked)) RtlUnwind(PVOID pEndFrame, PVOID targetIp, PEXCEPTION_RECORD pRecord, PVOID retval);
asm(    ".global _RtlUnwind\n"
        "_RtlUnwind:\n"
        "jmp _RtlUnwind_real\n");

// hacks around mingw
#undef InterlockedExchange
LONG WINAPI InterlockedExchange(LONG volatile *target, LONG value) {
    return __atomic_exchange_n(target, value, __ATOMIC_SEQ_CST);
}

#undef InterlockedIncrement
LONG WINAPI InterlockedIncrement(LONG volatile *target) {
    return __atomic_add_fetch(target, 1, __ATOMIC_SEQ_CST);
}

#undef InterlockedDecrement
LONG WINAPI InterlockedDecrement(LONG volatile *target) {
    return __atomic_sub_fetch(target, 1, __ATOMIC_SEQ_CST);
}

typedef struct {
    khandle_t sem;
    volatile _Atomic(uint32_t) owner;
} win32_mutex_t;

HANDLE WINAPI CreateMutexA(LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCSTR lpName) {
    kprintf("CreateMutexA(%p, %d, %p)", lpMutexAttributes, bInitialOwner, lpName);
    if (lpMutexAttributes != NULL || lpName != NULL || bInitialOwner)
        goto bad;
    
    win32_mutex_t* mut = rpmalloc(sizeof(win32_mutex_t));
    mut->sem = ksem_alloc(1);
    mut->owner = 0;
    
    return (HANDLE)kht_dummy_new(KHT_DUMMY_MUTEX, (uint32_t)mut);
    
bad:
    kpanicf("CreateMutexA(%p, %d, %p)", lpMutexAttributes, bInitialOwner, lpName);
}

BOOL WINAPI ReleaseMutex(HANDLE hMutex) {
    kprintf("ReleaseMutex(%p)", hMutex);
    win32_mutex_t* this = (win32_mutex_t*)kht_dummy_get((khandle_t)hMutex, KHT_DUMMY_MUTEX);
    this->owner = 0;
    ksem_post(this->sem);
    //kprintf("ReleaseMutex(%p)", hMutex);
    return TRUE;
}

typedef struct {
    int is_manual_reset;
    char* name;
    khandle_t kmut;
    khandle_t kcond;
    int state;
    
} win32_event_t;

HANDLE WINAPI CreateEventA(LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL bManualReset, BOOL bInitialState, LPCSTR lpName) {
    if (lpEventAttributes != NULL) {
        kpanicf("CreateEventA(%p, %d, %d, %p)", lpEventAttributes, bManualReset, bInitialState, lpName);
    }
    win32_event_t* new_event = rpmalloc(sizeof(win32_event_t));
    if (new_event == NULL) kpanic("can't allocate win32_event_t");
    new_event->is_manual_reset = bManualReset != 0;
    if (lpName == NULL)
        new_event->name = NULL;
    else {
        kprintf("allocating named event: %s", lpName);
        new_event->name = strdup(lpName);
        if (new_event->name == NULL)
            kpanic("can't allocate win32_event_t.name");
    }
    new_event->state = bInitialState != 0;
    new_event->kmut = kmut_alloc();
    new_event->kcond = kcond_alloc();
    
    return (HANDLE)kht_dummy_new(KHT_DUMMY_EVENT, (uint32_t)new_event);
}

static int win32_event_wait(win32_event_t* this, uint32_t timeout_ms) {
    kprintf("win32_event_wait(%p, %u)", this, timeout_ms);
    uint64_t timeout_us = timeout_ms * 1000;
    
    uint64_t time_start = kget_monotonic_time();
    uint64_t time_now = time_start; 
    
    int succeed = 0;
    
    kmut_lock(this->kmut, -1); // should be fast, timeout can be ignored
    if (this->is_manual_reset) {
        while (!this->state) {
            if (timeout_ms == INFINITE)
                kcond_wait(this->kcond, this->kmut, -1);
            else {
                kcond_wait(this->kcond, this->kmut, timeout_us - (time_now - time_start));
                time_now = kget_monotonic_time();
                if (time_now - time_start >= timeout_us)
                    break;
            }
        }
        succeed = 1;
    } else {
        kpanic("auto_reset_wait not implemented");
    }
    kmut_unlock(this->kmut);
    
    return succeed ? 0 : -1;
}

static void win32_event_free(win32_event_t* this) {
    rpfree(this->name);
    kht_delref(this->kmut, NULL, NULL);
    kht_delref(this->kcond, NULL, NULL);
    rpfree(this);
}

BOOL WINAPI SetEvent(HANDLE hEvent) {
    win32_event_t* this = (win32_event_t*)kht_dummy_get((khandle_t)hEvent, KHT_DUMMY_EVENT);
    kmut_lock(this->kmut, -1);
    if (this->is_manual_reset) {
        this->state = 1;
        kcond_broadcast(this->kcond);
    } else {
        kpanicf("SetEvent(%p), is_manual_reset = %d", hEvent, this->is_manual_reset);
    }
    kmut_unlock(this->kmut);
}

DWORD WINAPI GetCurrentDirectoryA(DWORD nBufferLength, LPSTR lpBuffer) {
    char buffer[4096];
    kfile_get_current_directory(buffer, sizeof(buffer));
    
    int32_t sz = strlen(buffer) + 1;
    
    for (int i = 0; i < sz - 1; i++)
        if (buffer[i] == '/')
            buffer[i] = '\\';
    
    kprintf("GetCurrentDirectoryA() -> %s", buffer);
        
    if (nBufferLength >= sz) {
        memcpy(lpBuffer, buffer, sz);
    }
    return sz;
}

DWORD WINAPI GetFullPathNameA(LPCSTR lpFileName, DWORD nBufferLength, LPSTR lpBuffer, LPSTR *lpFilePart) {
    char* r;
    
    
    if (lpFileName[1] != ':') {
        char current_directory[4096];
        
        int l = strlen(current_directory);
        for (int i = 0; i < l; i++)
            if (current_directory[i] == '/')
                current_directory[i] = '\\';
            
        kfile_get_current_directory(current_directory, sizeof(current_directory));
        asprintf(&r, lpFileName[0] == '\\' ? "%s%s" : "%s\\%s", current_directory, lpFileName);
    } else {
        asprintf(&r, "%s", lpFileName);
    }
    
    size_t sz = strlen(r) + 1;
    
    if (sz > nBufferLength) {
        rpfree(r);
        return sz;
    }
    
    memcpy(lpBuffer, r, sz);
    rpfree(r);
    
    if (lpFilePart != NULL) {
        *lpFilePart = r + sz - 1;
        while (**lpFilePart != '\\')
            (*lpFilePart)--;
    }
    
    return sz - 1;
}

HANDLE WINAPI CreateFileA(LPCSTR lpFileName, DWORD dwDesiredAccess, DWORD dwShareMode, LPSECURITY_ATTRIBUTES lpSecurityAttributes, DWORD dwCreationDisposition, DWORD dwFlagsAndAttributes, HANDLE hTemplateFile) {
    if (lpSecurityAttributes != NULL) {
        //kprintf("%d %p %d", lpSecurityAttributes->nLength, lpSecurityAttributes->lpSecurityDescriptor, lpSecurityAttributes->bInheritHandle);
        if (sizeof(SECURITY_ATTRIBUTES) != lpSecurityAttributes->nLength)
            kpanic("weird SECURITY_ATTRIBUTES size supplied");
        if (lpSecurityAttributes->lpSecurityDescriptor != NULL)
            kpanic("security what?");
    }
    if (dwCreationDisposition != OPEN_EXISTING && dwCreationDisposition != CREATE_ALWAYS) {
        kprintf("unsupported dwCreationDisposition");
        goto bad;
    }
    if (hTemplateFile != NULL) {
        kprintf("template files are unsupported");
        goto bad;
    }
    int r = (dwDesiredAccess & GENERIC_READ) != 0;
    int w = (dwDesiredAccess & GENERIC_WRITE) != 0;
    
    int creat = 0;
    if (dwCreationDisposition == CREATE_ALWAYS)
        creat = KFILE_CREATE | KFILE_TRUNCATE;

    khandle_t res = kfile_open(lpFileName, (r ? KFILE_READ : 0) | (w ? KFILE_WRITE : 0) | creat);
    
    kprintf("open(%s, %c%c) -> %08x", lpFileName, r ? 'r' : '-', w ? 'w' : '-', res);
    
    if (res == (khandle_t)-1)
        return INVALID_HANDLE_VALUE;
    return (HANDLE)res;
bad:
    kpanicf("CreateFileA(%s, %x, %x, %p, %x, %x, %p)", lpFileName, dwDesiredAccess, dwShareMode, lpSecurityAttributes, dwCreationDisposition, dwFlagsAndAttributes, hTemplateFile);
}

BOOL WINAPI ReadFile(HANDLE hFile, LPVOID lpBuffer, DWORD nNumberOfBytesToRead, LPDWORD lpNumberOfBytesRead, LPOVERLAPPED lpOverlapped) {
    if (lpOverlapped != NULL)
        goto bad;
    
    int32_t read = kfile_read((khandle_t)hFile, lpBuffer, nNumberOfBytesToRead);
    if (read == 0) {
        //kprintf("either end-of-file reached or error occured. dunno what to do");
        //goto bad;
    }
    *lpNumberOfBytesRead = read;
    
    //kprintf("ReadFile(%p, %p, %x, %p, %p) -> %d", hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped, read);
    
    return TRUE;
    
bad:
    kpanicf("ReadFile(%p, %p, %x, %p, %p)", hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);
}

BOOL WINAPI WriteFile(HANDLE hFile, LPCVOID lpBuffer, DWORD nNumberOfBytesToWrite, LPDWORD lpNumberOfBytesWritten, LPOVERLAPPED lpOverlapped) {
    if (lpOverlapped != NULL)
        goto bad;
    
    int32_t written = kfile_write((khandle_t)hFile, lpBuffer, nNumberOfBytesToWrite);
    *lpNumberOfBytesWritten = written;
    
    return TRUE;
bad:
    kpanicf("WriteFile(%p, %p, %x, %p, %p)", hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);
}

static uint32_t win32_move_method_to_korigin(DWORD dwMoveMethod) {
    switch (dwMoveMethod) {
        case FILE_BEGIN:    return KFILE_BGN;
        case FILE_CURRENT:  return KFILE_CUR;
        case FILE_END:      return KFILE_END;
        default:
            kpanicf("Unknown dwMoveMethod: %d", dwMoveMethod);
    }
}

DWORD WINAPI SetFilePointer(HANDLE hFile, LONG lDistanceToMove, PLONG lpDistanceToMoveHigh, DWORD dwMoveMethod) {
    if (lpDistanceToMoveHigh != NULL)
        goto bad;
    
    uint32_t origin = win32_move_method_to_korigin(dwMoveMethod);
    
    int32_t seek_res = kfile_seek((khandle_t)hFile, lDistanceToMove, origin);
    if (seek_res != 0)
        return INVALID_SET_FILE_POINTER;
    
    int32_t pos = kfile_tell((khandle_t)hFile);
    
    if (pos == -1)
        kpanicf("kfile_tell errored");
    
    //kprintf("SetFilePointer(%p, %x, %p, %x) -> %d", hFile, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod, pos);
    
    return pos;
bad:
    kpanicf("SetFilePointer(%p, %x, %p, %x)", hFile, lDistanceToMove, lpDistanceToMoveHigh, dwMoveMethod);
}

HANDLE WINAPI GetCurrentProcess() {
    return dummy_current_process_handle;
}

HANDLE WINAPI GetCurrentThread() {
    return (HANDLE)kthread_get_current_thread();//dummy_thread_handles[GetCurrentThreadId()];
}

BOOL WINAPI DuplicateHandle(HANDLE hSourceProcessHandle, HANDLE hSourceHandle, HANDLE hTargetProcessHandle, LPHANDLE lpTargetHandle, DWORD dwDesiredAccess, BOOL bInheritHandle, DWORD dwOptions) {
    if (hSourceProcessHandle != hTargetProcessHandle || hSourceProcessHandle != dummy_current_process_handle)
        goto bad;
    
    if (dwOptions & DUPLICATE_CLOSE_SOURCE) {
        kprint("close not supported");
        goto bad;
    }
    
    *lpTargetHandle = (HANDLE)kht_newref((uint32_t)hSourceHandle);
    
    return TRUE;
    
bad:
    kpanicf("bad DuplicateHandle");
}

DWORD WINAPI GetProfileStringA(LPCSTR lpAppName, LPCSTR lpKeyName, LPCSTR lpDefault, LPSTR lpReturnedString, DWORD nSize) {
    if (lpAppName == NULL || lpKeyName == NULL)
        goto bad;
    
    const char* result = "";
    if (lpDefault != NULL)
        result = lpDefault;
    
    size_t result_sz = strlen(result) + 1;
    if (result_sz > nSize)
        result_sz = nSize;
    
    memcpy(lpReturnedString, result, result_sz);
    
    return result_sz - 1;
    
bad:
    kpanicf("bad GetProfileStringA");
}

UINT WINAPI GetProfileIntA(LPCSTR lpAppName, LPCSTR lpKeyName, INT nDefault) {
    if (lpAppName == NULL || lpKeyName == NULL)
        goto bad;
    
    return nDefault;
bad:
    kpanicf("bad GetProfileIntA");
}

HGLOBAL WINAPI GlobalAlloc(UINT uFlags, SIZE_T dwBytes) {
    if (uFlags & GMEM_MOVEABLE) {
        goto bad;
    }
    
    //kprintf("GlobalAlloc(%x, %d)", uFlags, dwBytes);
    
    void* mem = rpmalloc(dwBytes);
    if (mem == NULL)
        kpanic("no mem for GlobalAlloc");
    
    if (uFlags & GMEM_ZEROINIT)
        memset(mem, 0, dwBytes);
    
    return mem;
    
bad:
    kpanicf("bad GlobalAlloc");
}

LPVOID WINAPI GlobalLock(HGLOBAL hMem) {
    return hMem;
}

int is_mask(const char* s) {
    while (*s != '\0') {
        if (*s == '*' || *s == '?')
            return 1;
        s++;
    }
    return 0;
}

typedef struct {
    khandle_t dir_handle;
    char* filename_mask;
    char* path;
} win32_search_t;

static void win32_unix_time_to_filetime(uint64_t unix_time, FILETIME *ft) {
    LONGLONG ll = unix_time * 10000000 + 116444736000000000;
    ft->dwLowDateTime = (DWORD)ll;
    ft->dwHighDateTime = ll >> 32;
}

static int win32_nostar_mask_test(const char* mask, const char* haystack) {
    const char* p1 = mask;
    const char* p2 = haystack;
    
    while (tolower(*p1) == tolower(*p2) || *p1 == '?') {
        p1++;
        p2++;
        if (*p1 == 0 || *p2 == 0) {
            if (*p1 == 0 && *p2 == 0)
                return 1;
            return 0;
        }
    }
    return 0;
}

static int win32_mask_test(const char* mask, const char* haystack) {
    // FUCK. This is some serious shit coming from the dos days...
    // https://docs.microsoft.com/en-us/archive/blogs/jeremykuhne/wildcards-in-windows
    // https://referencesource.microsoft.com/#System/services/io/system/io/PatternMatcher.cs
    
    kprintf("masktest %s vs %s", haystack, mask);
    
    int masklen = strlen(mask), haystacklen = strlen(haystack);
    if (masklen == 0)
        return 0;
    
    if (!strcmp(mask, "*") || !strcmp(mask, "*.*"))
        return 1;
    
    if (mask[0] == '*' && strrchr(mask + 1, '*') == NULL) {
        int rightLength = masklen - 1;
        return haystacklen >= rightLength && win32_nostar_mask_test(mask + 1, haystack + (haystacklen - rightLength));
    }
    
    kpanicf("don't know what to do with this mask: %s", mask);
}

static int win32_readdir(win32_search_t* search, LPWIN32_FIND_DATAA lpFindFileData) {
    char name_buffer[MAX_PATH];
    
    while (!kfile_readdir(search->dir_handle, name_buffer, sizeof(name_buffer))) {
        if (win32_mask_test(search->filename_mask, name_buffer)) {
            kfile_stat_t stat;
            
            char full_buffer[4096];
            snprintf(full_buffer, sizeof(full_buffer), "%s/%s", search->path, name_buffer);
            
            kfile_stat(full_buffer, &stat);
            
            memset(lpFindFileData, 0, sizeof(WIN32_FIND_DATAA));
            
            if (stat.mode == KFILE_MODE_FILE)
                lpFindFileData->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
            else
                lpFindFileData->dwFileAttributes = FILE_ATTRIBUTE_DIRECTORY;
            
            win32_unix_time_to_filetime(stat.atime, &lpFindFileData->ftLastAccessTime);
            win32_unix_time_to_filetime(stat.mtime, &lpFindFileData->ftLastWriteTime);
            win32_unix_time_to_filetime(stat.ctime, &lpFindFileData->ftCreationTime);
            
            lpFindFileData->nFileSizeLow = stat.size & 0xffffffff;
            lpFindFileData->nFileSizeHigh = (stat.size >> 32) & 0xffffffff;
            
            snprintf(lpFindFileData->cFileName, sizeof(lpFindFileData->cFileName), "%s", name_buffer);
            
            return 1;
        }
    }
    
    return 0;
}

static void win32_free_search(win32_search_t* search) {
    kht_delref(search->dir_handle, NULL, NULL);
    rpfree(search->filename_mask);
    rpfree(search->path);
    rpfree(search);
}

HANDLE WINAPI FindFirstFileA(LPCSTR lpFileName, LPWIN32_FIND_DATAA lpFindFileData) {
    kprintf("FindFirstFileA(%s, %p) err-stub called", lpFileName, lpFindFileData);
    
    // we want to split the filename into filename (which would include the mask) and path (mask in which is unsupported)
    
    int l = strlen(lpFileName);
    
    if (l == 0)
        goto bad;
    
    while (l > 0 && lpFileName[l] != '/' && lpFileName[l] != '\\')
        l--;
    
    char* filename, *path;
    if (l == 0) {
        path = strdup(".");
        if (lpFileName[0] == '/' || lpFileName[0] == '\\')
            filename = strdup(lpFileName + 1);
        else
            filename = strdup(lpFileName);
    } else {
        filename = strdup(lpFileName + l + 1);
        path = rpmalloc(l + 1);
        memcpy(path, lpFileName, l);
        path[l] = '\0';
        if (path == NULL)
            kpanic("no memory for search");
    }
    
    kprintf("path = %s; filename = %s", path, filename);
    
    if (is_mask(path) || !is_mask(filename))
        goto bad;
    
    win32_search_t* search = rpmalloc(sizeof(win32_search_t));
    if (search == NULL)
        kpanic("no memory for search");
    
    search->dir_handle = kfile_opendir(path);
    search->filename_mask = filename;
    search->path = path;
    
    if (win32_readdir(search, lpFindFileData)) {
        return (HANDLE)search;
    } else {
        win32_free_search(search);
        SetLastError(ERROR_FILE_NOT_FOUND);
        return INVALID_HANDLE_VALUE;
    }
bad:
    kpanic("bad FindFirstFileA");
}

BOOL WINAPI FindNextFileA(HANDLE hFindFile, LPWIN32_FIND_DATAA lpFindFileData) {
    win32_search_t* search = hFindFile;
    
    if (win32_readdir(search, lpFindFileData)) {
        return TRUE;
    } else {
        SetLastError(ERROR_FILE_NOT_FOUND);
        return FALSE;
    }
}

BOOL WINAPI FindClose(HANDLE hFindFile) {
    win32_free_search(hFindFile);
    return TRUE;
}

DWORD WINAPI GetCurrentProcessId() {
    return 2;
}

static UINT error_mode;

UINT WINAPI SetErrorMode(UINT uMode) {
    return __atomic_exchange_n(&error_mode, uMode, __ATOMIC_SEQ_CST);
}

typedef BOOL WINAPI (*PDllMain)(
  _In_ HINSTANCE hinstDLL,
  _In_ DWORD     fdwReason,
  _In_ LPVOID    lpvReserved);

HMODULE WINAPI LoadLibraryA(LPCSTR lpLibFileName) {
    HMODULE loaded = GetModuleHandleA(lpLibFileName);
    
    if (loaded) {
        return loaded;
    } else if (!strcmp(lpLibFileName, "DSOUND.DLL")) {
        kprintf("I don't have DSOUND.DLL, I swear");
        return NULL;
    } 
    
    
    kprintf("LoadLibraryA(%s)", lpLibFileName);
    HMODULE mod = (HMODULE)win32_load_library(lpLibFileName);
    if (mod == NULL)
        return mod;
    
    // call DllMain with DLL_PROCESS_ATTACH
    PDllMain dll_main = win32_get_entry_point((uint32_t)mod);
    
    if (!dll_main((HINSTANCE)mod, DLL_PROCESS_ATTACH, NULL))
        kpanicf("DllMain for %s failed", lpLibFileName);
    
    return mod;
bad:
    kpanicf("LoadLibraryA(%s)", lpLibFileName);
}

VOID WINAPI Sleep(DWORD dwMilliseconds) {
    if (dwMilliseconds)
        ksleep(dwMilliseconds);
    
    kprintf("Sleep(%d)", dwMilliseconds);
}

HGLOBAL WINAPI GlobalHandle(LPCVOID pMem) {
    return (HGLOBAL)pMem;
}

BOOL WINAPI GlobalUnlock(HGLOBAL hMem) {
    return TRUE;
}

HGLOBAL WINAPI GlobalFree(HGLOBAL hMem) {
    rpfree(hMem);
    return NULL;
}

DWORD WINAPI SuspendThread(HANDLE hThread) {
    kprintf("SuspendThread(%p)", hThread);
    return kthread_suspend((khandle_t)hThread);
}

DWORD WINAPI ResumeThread(HANDLE hThread) {
    kprintf("ResumeThread(%p)", hThread);
    return kthread_resume((khandle_t)hThread);
}


HANDLE WINAPI CreateThread(LPSECURITY_ATTRIBUTES lpThreadAttributes, SIZE_T dwStackSize, LPTHREAD_START_ROUTINE lpStartAddress, LPVOID lpParameter, DWORD dwCreationFlags, LPDWORD lpThreadId) {
    if (lpThreadAttributes != NULL) {
        kprint("security what?");
        goto bad;
    }
    
    if (dwStackSize == 0)
        dwStackSize = 1024 * 1024; // 1 MiB
        
    if (dwCreationFlags & ~CREATE_SUSPENDED != 0)
        goto bad;
    
    uint32_t suspended = (dwCreationFlags & CREATE_SUSPENDED) != 0;
    
    khandle_t thread = kthread_create((kthread_entry_t)lpStartAddress, lpParameter, dwStackSize, suspended);
    
    if (lpThreadId != NULL)
        *lpThreadId = kthread_get_id(thread);
    
    return (HANDLE)thread;
    
bad:
    kpanicf("CreateThread(%p, %x, %p, %x, %x, %p)", lpThreadAttributes, dwStackSize, lpStartAddress, lpParameter, dwCreationFlags, lpThreadId);
}

BOOL WINAPI SetThreadPriority(HANDLE hThread, int nPriority) {
    
    kprintf("SetThreadPriority(%p, %d) no-op stub called", hThread, nPriority);
    
    return TRUE;
}

DWORD WINAPI WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds) {
    uint32_t type = kht_gettype((khandle_t)hHandle);
    //kprintf("obj type = %d", type);
    if (type == KHT_DUMMY) {
        uint32_t dummy_type = kht_dummy_gettype((khandle_t)hHandle);
        //kprintf("dummy type = %d", dummy_type);
        
        switch (dummy_type) {
            case KHT_DUMMY_EVENT:
            {
                win32_event_t* event = (win32_event_t*)kht_dummy_get((khandle_t)hHandle, KHT_DUMMY_EVENT);
                if (win32_event_wait(event, dwMilliseconds))
                    return WAIT_TIMEOUT;
                return WAIT_OBJECT_0;
            }
            case KHT_DUMMY_MUTEX:
            {
                kpanic("mutex implementation is broken");
                uint64_t timeout = (uint64_t)dwMilliseconds * 1000;
                if (dwMilliseconds == INFINITE)
                    timeout = KSEM_TMO_INF;
                win32_mutex_t* mutex = (win32_mutex_t*)kht_dummy_get((khandle_t)hHandle, KHT_DUMMY_MUTEX);
                //kprintf("wait_mutex(%p)", hHandle);
                khandle_t my_thread = kthread_get_current_thread();
                //kprintf("my_thread = %x", my_thread);
                if (my_thread != mutex->owner) {
                    //kprint("wait...");
                    if (ksem_wait(mutex->sem, timeout)) {
                        //kprintf("sem tmo...");
                        return WAIT_TIMEOUT;
                    }
                    //kprint("now own");
                    mutex->owner = my_thread;
                    return WAIT_OBJECT_0;
                }
                //kprint("already owned");
                return WAIT_OBJECT_0;
            }
            default:
                goto bad;
        }
    } else if (type == KHT_THREAD) {
        if (dwMilliseconds != INFINITE)
            goto bad;
        kthread_wait((khandle_t)hHandle);
        return WAIT_OBJECT_0;
    } else if (type == KHT_SEM) {
        //kprintf("WaitForSingleObject(%p, %u)", hHandle, dwMilliseconds);
        uint64_t timeout = (uint64_t)dwMilliseconds * 1000;
        if (dwMilliseconds == INFINITE)
            timeout = KSEM_TMO_INF;
        if (ksem_wait((khandle_t)hHandle, timeout)) {
            kprintf("sem timeout");
            return WAIT_TIMEOUT;
        }
        return WAIT_OBJECT_0;
    }
    
bad:
    kpanicf("WaitForSingleObject(%p, %d)", hHandle, dwMilliseconds);
}

// slow hacks...
DWORD WINAPI WaitForMultipleObjects(DWORD nCount, const HANDLE *lpHandles, BOOL bWaitAll, DWORD dwMilliseconds) {
    if (dwMilliseconds != INFINITE)
        goto bad;
    
    if (!bWaitAll) {
        while (1) {
            for (int i = 0; i < nCount; i++) {
                if (WaitForSingleObject(lpHandles[i], 10) == WAIT_OBJECT_0)
                    return WAIT_OBJECT_0 + i;
            }
        }
    }
bad:
    kpanicf("WaitForMultipleObjects(%d, %p, %d, %d)", nCount, lpHandles, bWaitAll, dwMilliseconds);
}

LPSTR WINAPI lstrcatA(LPSTR lpString1, LPCSTR lpString2) {
    strcat(lpString1, lpString2);
    return lpString1;
}

DWORD WINAPI GetTickCount(VOID) {
    uint64_t diff = kget_monotonic_time() - start_time;
    
    return diff / 1000 + 10 * 60 * 1000;// +10m
}

BOOL WINAPI GetDiskFreeSpaceA(LPCSTR lpRootPathName, LPDWORD lpSectorsPerCluster, LPDWORD lpBytesPerSector, LPDWORD lpNumberOfFreeClusters, LPDWORD lpTotalNumberOfClusters) {
    
    if (lpRootPathName != NULL)
        goto bad;
    
    uint64_t free_bytes, total_bytes;
    
    kfile_get_free_space(&free_bytes, &total_bytes);
    
    *lpSectorsPerCluster = 8;
    *lpBytesPerSector = 512;
    *lpNumberOfFreeClusters = free_bytes / 4096;
    *lpTotalNumberOfClusters = total_bytes / 4096;
    
    return TRUE;
    
bad:
    kpanicf("GetDiskFreeSpaceA(%p, %p, %p, %p, %p)", lpRootPathName, lpRootPathName ? lpRootPathName : "", lpSectorsPerCluster, lpBytesPerSector, lpNumberOfFreeClusters, lpTotalNumberOfClusters);
}

BOOL WINAPI CloseHandle(HANDLE hObject) {
    uint32_t dummy_type, dummy_value;
    kht_delref((khandle_t)hObject, &dummy_type, &dummy_value);
    
    switch (dummy_type) {
        case 0:
            break;
        case KHT_DUMMY_EVENT:
            win32_event_free((win32_event_t*)dummy_value);
            break;
    
        default:
            kpanicf("don't know how to free dummy %u %u", dummy_type, dummy_value);
    }
    
    //kprintf("CloseHandle(%p) executed as no-op", hObject);
    
    return TRUE;
}

void WINAPI ExitThread(DWORD dwExitCode) {
    kprintf("ExitThread(%u)", (unsigned)dwExitCode);
    kthread_leave();
    kpanic("kthread_leave did not work...");
}

BOOL WINAPI SetCurrentDirectoryA(LPCSTR lpPathName) {
    //kpanicf("SetCurrentDirectoryA(%s)", lpPathName);
    
    if (kfile_set_current_directory(lpPathName))
        return FALSE;
    
    return TRUE;
}

BOOL WINAPI SetEnvironmentVariableA(LPCSTR lpName, LPCSTR lpValue) {
    kprintf("SetEnvironmentVariableA(%s, %s) executed as no-op", lpName, lpValue);
    return TRUE;
}


#define TICKSPERMIN        600000000
#define TICKSPERSEC        10000000
#define TICKSPERMSEC       10000
#define SECSPERDAY         86400
#define SECSPERHOUR        3600
#define SECSPERMIN         60
#define MINSPERHOUR        60
#define HOURSPERDAY        24
#define EPOCHWEEKDAY       1
#define DAYSPERWEEK        7
#define EPOCHYEAR          1601
#define DAYSPERNORMALYEAR  365
#define DAYSPERLEAPYEAR    366
#define MONSPERYEAR        12

DWORD WINAPI GetTimeZoneInformation(LPTIME_ZONE_INFORMATION lpTimeZoneInformation) {
    memset(lpTimeZoneInformation, 0, sizeof(TIME_ZONE_INFORMATION));
    lpTimeZoneInformation->Bias = -kget_timezone_offset() / 60;
    return TIME_ZONE_ID_UNKNOWN;
}

BOOL WINAPI FileTimeToLocalFileTime(const FILETIME *lpFileTime, LPFILETIME lpLocalFileTime) {
    LARGE_INTEGER liTime;

    liTime.u.LowPart = lpFileTime->dwLowDateTime;
    liTime.u.HighPart = lpFileTime->dwHighDateTime;
    if (liTime.QuadPart < 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    uint64_t bias = (uint64_t)kget_timezone_offset() * TICKSPERSEC;
    
    liTime.QuadPart += bias;
    
    lpLocalFileTime->dwLowDateTime = liTime.u.LowPart;
    lpLocalFileTime->dwHighDateTime = liTime.u.HighPart;
    
    return TRUE;
}

// proudly borrowed from ReactOS

typedef struct _TIME_FIELDS {
    USHORT Year;        // range [1601...]
    USHORT Month;       // range [1..12]
    USHORT Day;         // range [1..31]
    USHORT Hour;        // range [0..23]
    USHORT Minute;      // range [0..59]
    USHORT Second;      // range [0..59]
    USHORT Milliseconds;// range [0..999]
    USHORT Weekday;     // range [0..6] == [Sunday..Saturday]
} TIME_FIELDS;
typedef TIME_FIELDS *PTIME_FIELDS;

static const unsigned int YearLengths[2] =
{
    DAYSPERNORMALYEAR, DAYSPERLEAPYEAR
};
static const UCHAR MonthLengths[2][MONSPERYEAR] =
{
    { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
    { 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

 
static __inline int IsLeapYear(int Year)
{
    return Year % 4 == 0 && (Year % 100 != 0 || Year % 400 == 0) ? 1 : 0;
}

static int DaysSinceEpoch(int Year)
{
    int Days;
    Year--; /* Don't include a leap day from the current year */
    Days = Year * DAYSPERNORMALYEAR + Year / 4 - Year / 100 + Year / 400;
    Days -= (EPOCHYEAR - 1) * DAYSPERNORMALYEAR + (EPOCHYEAR - 1) / 4 - (EPOCHYEAR - 1) / 100 + (EPOCHYEAR - 1) / 400;
    return Days;
}

VOID RtlTimeToTimeFields(IN PLARGE_INTEGER Time, OUT PTIME_FIELDS TimeFields)
{
    const UCHAR *Months;
    ULONG SecondsInDay, CurYear;
    ULONG LeapYear, CurMonth;
    ULONG Days;
    ULONGLONG IntTime = Time->QuadPart;

    /* Extract millisecond from time and convert time into seconds */
    TimeFields->Milliseconds = (SHORT)((IntTime % TICKSPERSEC) / TICKSPERMSEC);
    IntTime = IntTime / TICKSPERSEC;

    /* Split the time into days and seconds within the day */
    Days = (ULONG)(IntTime / SECSPERDAY);
    SecondsInDay = IntTime % SECSPERDAY;

    /* Compute time of day */
    TimeFields->Hour = (SHORT)(SecondsInDay / SECSPERHOUR);
    SecondsInDay = SecondsInDay % SECSPERHOUR;
    TimeFields->Minute = (SHORT)(SecondsInDay / SECSPERMIN);
    TimeFields->Second = (SHORT)(SecondsInDay % SECSPERMIN);

    /* Compute day of week */
    TimeFields->Weekday = (SHORT)((EPOCHWEEKDAY + Days) % DAYSPERWEEK);

    /* Compute year */
    CurYear = EPOCHYEAR;
    CurYear += Days / DAYSPERLEAPYEAR;
    Days -= DaysSinceEpoch(CurYear);
    while (TRUE)
    {
        LeapYear = IsLeapYear(CurYear);
        if (Days < YearLengths[LeapYear])
        {
            break;
        }
        CurYear++;
        Days = Days - YearLengths[LeapYear];
    }
    TimeFields->Year = (SHORT)CurYear;

    /* Compute month of year */
    LeapYear = IsLeapYear(CurYear);
    Months = MonthLengths[LeapYear];
    for (CurMonth = 0; Days >= Months[CurMonth]; CurMonth++)
    {
        Days = Days - Months[CurMonth];
    }
    TimeFields->Month = (SHORT)(CurMonth + 1);
    TimeFields->Day = (SHORT)(Days + 1);
}

BOOL WINAPI FileTimeToSystemTime(const FILETIME *lpFileTime, LPSYSTEMTIME lpSystemTime) {
    TIME_FIELDS TimeFields;
    LARGE_INTEGER liTime;

    liTime.u.LowPart = lpFileTime->dwLowDateTime;
    liTime.u.HighPart = lpFileTime->dwHighDateTime;
    if (liTime.QuadPart < 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    RtlTimeToTimeFields(&liTime, &TimeFields);

    lpSystemTime->wYear = TimeFields.Year;
    lpSystemTime->wMonth = TimeFields.Month;
    lpSystemTime->wDay = TimeFields.Day;
    lpSystemTime->wHour = TimeFields.Hour;
    lpSystemTime->wMinute = TimeFields.Minute;
    lpSystemTime->wSecond = TimeFields.Second;
    lpSystemTime->wMilliseconds = TimeFields.Milliseconds;
    lpSystemTime->wDayOfWeek = TimeFields.Weekday;

    return TRUE;
}

int WINAPI WideCharToMultiByte(UINT CodePage, DWORD dwFlags, LPCWCH lpWideCharStr, int cchWideChar, LPSTR lpMultiByteStr, int cbMultiByte, LPCCH lpDefaultChar, LPBOOL lpUsedDefaultChar) {
    kprintf("WideCharToMultiByte(%u, %x, %p, %d, %p, %d, %p, %p) err-stub called", CodePage, dwFlags, lpWideCharStr, cchWideChar, lpMultiByteStr, cbMultiByte, lpDefaultChar, lpUsedDefaultChar);
    SetLastError(ERROR_INVALID_FLAGS); // whatever
    return 0;
}

BOOL WINAPI GetFileTime(HANDLE hFile, LPFILETIME lpCreationTime, LPFILETIME lpLastAccessTime, LPFILETIME lpLastWriteTime) {
    kfile_stat_t stat;
    kfile_fstat((khandle_t)hFile, &stat);
    
    if (lpCreationTime != NULL)
        win32_unix_time_to_filetime(stat.atime, lpCreationTime);
    if (lpLastAccessTime != NULL)
        win32_unix_time_to_filetime(stat.mtime, lpLastAccessTime);
    if (lpLastWriteTime != NULL)
        win32_unix_time_to_filetime(stat.ctime, lpLastWriteTime);
    
    return TRUE;
}

void WINAPI GetLocalTime(LPSYSTEMTIME lpSystemTime) {
    FILETIME ft;
    
    win32_unix_time_to_filetime(kget_real_time() + kget_timezone_offset() * (uint64_t)1000000, &ft);
    FileTimeToSystemTime(&ft, lpSystemTime);
}

void WINAPI GetSystemTime(LPSYSTEMTIME lpSystemTime) {
    FILETIME ft;
    
    win32_unix_time_to_filetime(kget_real_time(), &ft);
    FileTimeToSystemTime(&ft, lpSystemTime);
}

void WINAPI ExitProcess(UINT uExitCode) {
    kprintf("ExitProcess(%u)", (unsigned)uExitCode);
    kexit();
}

BOOL WINAPI IsProcessorFeaturePresent(DWORD ProcessorFeature) {
    if (ProcessorFeature == PF_FLOATING_POINT_PRECISION_ERRATA)
        return FALSE;
    
    kpanicf("IsProcessorFeaturePresent(%x)", ProcessorFeature);
}
