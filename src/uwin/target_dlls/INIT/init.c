/*
 *  entry to emulated code
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

#include <ksvc.h>
#include <windows.h>
#include <rpmalloc.h>

typedef BOOL WINAPI (*PDllMain)(
  _In_ HINSTANCE hinstDLL,
  _In_ DWORD     fdwReason,
  _In_ LPVOID    lpvReserved);

typedef struct {
    HINSTANCE hinst;
    PDllMain routine;
} library_init_routine_t;

static library_init_routine_t* lib_init_routines = NULL;
typedef void (*main_entry_point_t)(void);
typedef uint32_t __stdcall (*thread_entry_point_t)(void*);

static void call_dll_entries()
{
    library_init_routine_t* pdll = lib_init_routines;
    while (pdll->hinst != NULL) {
        kprintf("calling init for %p %p", pdll->hinst, pdll->routine);
        BOOL res = pdll->routine(pdll->hinst, DLL_PROCESS_ATTACH, NULL);
        if (!res) {
            kprintf("init for %p %p failed", pdll->hinst, pdll->routine);
        }
        
        pdll++;
    }
}

const rpmalloc_config_t my_rpmalloc_config = {
    NULL, NULL, 0x1000, 64 * 1024, 64, 0
};

void __stdcall init(void* entry_point, void* entry_param)
{
    kprintf("==================\nstart init\n==================");
    
    kprintf("entry_point = %p, entry_param = %p", entry_point, entry_param);
    
    uint32_t pid, tid;
    pid = (uint32_t)get_teb_value(TEB_PID);
    tid = (uint32_t)get_teb_value(TEB_TID);
    
    kprintf("pid = %x, tid = %x", pid, tid);
    kprintf("initializing FPU");
    __asm__("finit");

    if (tid == KTHREAD_INIT_ID) {
        
        lib_init_routines = get_teb_value(TEB_INITLIST);
        
        if (lib_init_routines == NULL)
            kprintf("lib_init_routines is NULL");
        
        kprintf("initialising rpmalloc...");
        rpmalloc_initialize_config(&my_rpmalloc_config);
        
        kprintf("initialiing DLLs...");
        call_dll_entries();
        
        kprintf("calling main entry point");
        ((main_entry_point_t)entry_point)();
    } else {
        // TODO: maybe call dlls with DLL_THREAD_ATTACH?
        kprintf("initializing rpmalloc for the thread...");
        rpmalloc_thread_initialize();
        
        kprintf("calling thread entry point");
        uint32_t r = ((thread_entry_point_t)entry_point)(entry_param);
        kprintf("thread entry for %d returned %u.", tid, r);
        kthread_leave(); // TODO: return code?
    }
    
    kpanic("init end reached");
}
