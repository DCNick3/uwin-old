/*
 *  kernel services (provided by code running on host mostly)
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

#include <stdint.h>
#include <stdarg.h>
#include <limits.h>

#include "ksvc.h"

#include "printf.h"
#include "rpmalloc.h"

void *alloca(size_t size);

// TODO add regparm attribute to kcall* functions (for faster syscalls)

int32_t KSVC_DLL kcall0(uint32_t knum)
{
    int32_t r;
    asm volatile("mov %1, %%eax\n"
        "int $0x80\n"
        "mov %%eax, %0\n"
        : "=m"(r)
        : "m"(knum)
        : "eax"
    );
    return r;
}
int32_t KSVC_DLL kcall1(uint32_t knum, uint32_t arg0)
{
    int32_t r;
    asm volatile("mov %1, %%eax\n"
        "mov %2, %%ebx\n"
        "int $0x80\n"
        "mov %%eax, %0\n"
        : "=m"(r)
        : "m"(knum), "m"(arg0)
        : "eax", "ebx"
    );
    return r;
}
int32_t KSVC_DLL kcall2(uint32_t knum, uint32_t arg0, uint32_t arg1)
{
    int32_t r;
    asm volatile("mov %1, %%eax\n"
        "mov %2, %%ebx\n"
        "mov %3, %%ecx\n"
        "int $0x80\n"
        "mov %%eax, %0\n"
        : "=m"(r)
        : "m"(knum), "m"(arg0), "m"(arg1)
        : "eax", "ebx", "ecx"
    );
    return r;
}

int32_t KSVC_DLL kcall3(uint32_t knum, uint32_t arg0, uint32_t arg1, uint32_t arg2)
{
    int32_t r;
    asm volatile("mov %1, %%eax\n"
        "mov %2, %%ebx\n"
        "mov %3, %%ecx\n"
        "mov %4, %%edx\n"
        "int $0x80\n"
        "mov %%eax, %0\n"
        : "=m"(r)
        : "m"(knum), "m"(arg0), "m"(arg1), "m"(arg2)
        : "eax", "ebx", "ecx", "edx"
    );
    return r;
}
int32_t KSVC_DLL kcall4(uint32_t knum, uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3)
{
    int32_t r;
    asm volatile("mov %1, %%eax\n"
        "mov %2, %%ebx\n"
        "mov %3, %%ecx\n"
        "mov %4, %%edx\n"
        "mov %5, %%esi\n"
        "int $0x80\n"
        "mov %%eax, %0\n"
        : "=m"(r)
        : "m"(knum), "m"(arg0), "m"(arg1), "m"(arg2), "m"(arg3)
        : "eax", "ebx", "ecx", "edx", "esi"
    );
    return r;
}
int32_t KSVC_DLL kcall5(uint32_t knum, uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4)
{
    int32_t r;
    asm volatile("mov %1, %%eax\n"
        "mov %2, %%ebx\n"
        "mov %3, %%ecx\n"
        "mov %4, %%edx\n"
        "mov %5, %%esi\n"
        "mov %6, %%edi\n"
        "int $0x80\n"
        "mov %%eax, %0\n"
        : "=m"(r)
        : "m"(knum), "m"(arg0), "m"(arg1), "m"(arg2), "m"(arg3), "m"(arg4)
        : "eax", "ebx", "ecx", "edx", "esi", "edi"
    );
    return r;
}
int32_t  KSVC_DLL kcall6(uint32_t knum, uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5)
{
    int32_t r;
    asm volatile("mov %1, %%eax\n"
        "mov %2, %%ebx\n"
        "mov %3, %%ecx\n"
        "mov %4, %%edx\n"
        "mov %5, %%esi\n"
        "mov %6, %%edi\n"
        "mov %7, %%ebp\n"
        "int $0x80\n"
        "mov %%eax, %0\n"
        : "=m"(r)
        : "m"(knum), "m"(arg0), "m"(arg1), "m"(arg2), "m"(arg3), "m"(arg4), "m"(arg5)
        : "eax", "ebx", "ecx", "edx", "esi", "edi", "ebp"
    );
    return r;
}


void KSVC_DLL __attribute__((noreturn)) kexit(void)
{
    kcall0(SYSCALL_EXIT);
    while(1) ;
    //__builtin_unreachable();
}

void KSVC_DLL __attribute__((noreturn)) kpanic(const char *message)
{
    kcall1(SYSCALL_PANIC, (uint32_t)message);
    while(1) ;
    //__builtin_unreachable();
}
void KSVC_DLL __attribute__((noreturn)) kpanicf(const char *format, ...)
{
    char buffer[4096];
    va_list va;
    va_start(va, format);;
    int ret = vsnprintf(buffer, sizeof(buffer), format, va);
    kpanic(buffer);
}

void KSVC_DLL kprint(const char* message)
{
    kcall1(SYSCALL_PRINT, (uint32_t)message);
}
int KSVC_DLL vkprintf(const char* format, va_list va)
{
    char buffer[4096];
    int ret = vsnprintf(buffer, sizeof(buffer), format, va);
    kprint(buffer);
    return ret;
}
int KSVC_DLL kprintf(const char* format, ...)
{
    va_list va;
    va_start(va, format);
    int ret = vkprintf(format, va);
    va_end(va);
    return ret;
}

void* KSVC_DLL kmap_memory_dynamic(uint32_t size, uint32_t prot)
{
    return (void*)kcall2(SYSCALL_MAP_MEMORY_DYNAMIC, size, prot);
}
uint32_t KSVC_DLL kmap_memory_fixed(void* address, uint32_t size, uint32_t prot)
{
    return (uint32_t)kcall3(SYSCALL_MAP_MEMORY_FIXED, (uint32_t)address, size, prot);
}
int32_t KSVC_DLL kunmap_memory(void* address, uint32_t size)
{
    return kcall2(SYSCALL_UNMAP_MEMORY, (uint32_t)address, size);
}
int32_t KSVC_DLL kprotect_memory(void* address, uint32_t size, uint32_t prot)
{
    return kcall3(SYSCALL_PROTECT_MEMORY, (uint32_t)address, size, prot);
}

uint64_t KSVC_DLL kget_monotonic_time()
{
    uint64_t res;
    kcall1(SYSCALL_GET_MONOTONIC_TIME, (uint32_t)&res);
    return res;
}
uint64_t KSVC_DLL kget_real_time()
{
    uint64_t res;
    kcall1(SYSCALL_GET_REAL_TIME, (uint32_t)&res);
    return res;
}
uint32_t KSVC_DLL kget_timezone_offset()
{
    return kcall0(SYSCALL_GET_TIMEZONE_OFFSET);
}
void KSVC_DLL ksleep(uint32_t time_ms)
{
    kcall1(SYSCALL_SLEEP, time_ms);
}

khandle_t KSVC_DLL kht_dummy_new(uint32_t type, uint32_t value)
{
    return kcall2(SYSCALL_HT_DUMMY_NEW, type, value);
}
uint32_t KSVC_DLL kht_dummy_get(khandle_t handle, uint32_t type)
{
    return kcall2(SYSCALL_HT_DUMMY_GET, handle, type);
}
uint32_t KSVC_DLL kht_dummy_gettype(khandle_t handle)
{
    return kcall1(SYSCALL_HT_DUMMY_GETTYPE, handle);
}
khandle_t KSVC_DLL kht_newref(khandle_t handle)
{
    return kcall1(SYSCALL_HT_NEWREF, handle);
}
void KSVC_DLL kht_delref(khandle_t handle, uint32_t* dummy_type, uint32_t* dummy_value)
{
    kcall3(SYSCALL_HT_DELREF, handle, (uint32_t)dummy_type, (uint32_t)dummy_value);
}
uint32_t KSVC_DLL kht_gettype(khandle_t handle)
{
    return kcall1(SYSCALL_HT_GETTYPE, handle);
}

khandle_t KSVC_DLL ksem_alloc(uint32_t value)
{
    return kcall1(SYSCALL_SEM_ALLOC, value);
}
void KSVC_DLL ksem_post(khandle_t handle)
{
    kcall1(SYSCALL_SEM_POST, handle);
}
int32_t KSVC_DLL ksem_wait(khandle_t handle, uint64_t timeout_us)
{
    // Well... It just works
    // passing timeout_us directly makes gcc generate weird code, this is how I dealt with it...
    uint64_t* p = alloca(8);
    *p = timeout_us;
    return kcall2(SYSCALL_SEM_WAIT, handle, (uint32_t)p);
}

khandle_t KSVC_DLL kmut_alloc()
{
    return kcall0(SYSCALL_MUT_ALLOC);
}
int32_t KSVC_DLL kmut_lock(khandle_t handle, uint64_t timeout_us)
{
    // Well... It just works
    // passing timeout_us directly makes gcc generate weird code, this is how I dealt with it...
    uint64_t* p = alloca(8);
    *p = timeout_us;
    return kcall2(SYSCALL_MUT_LOCK, handle, (uint32_t)p);
}
void KSVC_DLL kmut_unlock(khandle_t handle)
{
    kcall1(SYSCALL_MUT_UNLOCK, handle);
}

khandle_t KSVC_DLL kcond_alloc()
{
    return kcall0(SYSCALL_COND_ALLOC);
}
void KSVC_DLL kcond_signal(khandle_t handle)
{
    kcall1(SYSCALL_COND_SIGNAL, handle);
}
void KSVC_DLL kcond_broadcast(khandle_t handle)
{
    kcall1(SYSCALL_COND_BROADCAST, handle);
}
int32_t KSVC_DLL kcond_wait(khandle_t cond_handle, khandle_t mutex_handle, uint64_t timeout_us)
{
    // Well... It just works
    // passing timeout_us directly makes gcc generate weird code, this is how I dealt with it...
    uint64_t* p = alloca(8);
    *p = timeout_us;
    return kcall3(SYSCALL_COND_WAIT, cond_handle, mutex_handle, (uint32_t)p);
}

int32_t KSVC_DLL kcfg_read(const char* key, char* buffer, uint32_t buffer_size)
{
    return kcall3(SYSCALL_CFG_READ, (uint32_t)key, (uint32_t)buffer, buffer_size);
}
char* KSVC_DLL kcfg_read_all(const char* key)
{
    int32_t r = kcfg_read(key, NULL, 0);
    if (r == -1) return NULL;
    char* buf = rpmalloc(r);
    if (buf == NULL) kpanic("no memory for ksvc_read_all");
    kcfg_read(key, buf, r);
    return buf;
}
void KSVC_DLL kcfg_write(const char* key, const char* value)
{
    kcall2(SYSCALL_CFG_WRITE, (uint32_t)key, (uint32_t)value);
}
void KSVC_DLL kcfg_commit()
{
    kcall0(SYSCALL_CFG_COMMIT);
}

khandle_t KSVC_DLL kfile_open(const char* name, uint32_t mode)
{
    return kcall2(SYSCALL_FILE_OPEN, (uint32_t)name, mode);
}
int32_t KSVC_DLL kfile_read(khandle_t handle, char* buffer, uint32_t buffer_size)
{
    return kcall3(SYSCALL_FILE_READ, handle, (uint32_t)buffer, buffer_size);
}
int32_t KSVC_DLL kfile_write(khandle_t handle, const char* buffer, uint32_t buffer_size)
{
    return kcall3(SYSCALL_FILE_WRITE, handle, (uint32_t)buffer, buffer_size);
}
int32_t KSVC_DLL kfile_seek(khandle_t handle, int64_t seek, uint32_t origin)
{
    // Well... It just works
    // passing seek directly makes gcc generate weird code, this is how I dealt with it...
    int64_t* p = alloca(8);
    *p = seek;
    int32_t r = kcall3(SYSCALL_FILE_SEEK, handle, (uint32_t)p, origin);
    return r;
}
int64_t KSVC_DLL kfile_tell(khandle_t handle)
{
    int64_t r;
    kcall2(SYSCALL_FILE_TELL, handle, (int32_t)&r);
    return r;
}
void KSVC_DLL kfile_get_free_space(uint64_t *free_bytes, uint64_t *total_bytes)
{
    kcall2(SYSCALL_FILE_GET_FREE_SPACE, (uint32_t)free_bytes, (uint32_t)total_bytes);
}
int32_t KSVC_DLL kfile_set_current_directory(const char *new_current_directory)
{
    return kcall1(SYSCALL_FILE_SET_CURRENT_DIRECTORY, (uint32_t)new_current_directory);
}
void KSVC_DLL kfile_get_current_directory(char *buffer, uint32_t size)
{
    kcall2(SYSCALL_FILE_GET_CURRENT_DIRECTORY, (uint32_t)buffer, size);
}
void KSVC_DLL kfile_stat(const char* path, kfile_stat_t* stat)
{
    kcall2(SYSCALL_FILE_STAT, (uint32_t)path, (uint32_t)stat);
}
void KSVC_DLL kfile_fstat(khandle_t file, kfile_stat_t* stat)
{
    kcall2(SYSCALL_FILE_FSTAT, file, (uint32_t)stat);
}
khandle_t KSVC_DLL kfile_opendir(const char* path)
{
    return kcall1(SYSCALL_FILE_OPENDIR, (uint32_t)path);
}
int32_t KSVC_DLL kfile_readdir(khandle_t dir, char* buffer, uint32_t size)
{
    return kcall3(SYSCALL_FILE_READDIR, dir, (uint32_t)buffer, size);
}

khandle_t KSVC_DLL ksurf_alloc(int32_t width, uint32_t height)
{
    return kcall2(SYSCALL_SURF_ALLOC, width, height);
}
void KSVC_DLL ksurf_lock(khandle_t handle, ksurf_lockdesc_t* lockdesc)
{
    kcall2(SYSCALL_SURF_LOCK, handle, (uint32_t)lockdesc);
}
void KSVC_DLL ksurf_unlock(khandle_t handle)
{
    kcall1(SYSCALL_SURF_UNLOCK, handle);
}
void KSVC_DLL ksurf_blit(khandle_t src, const krect_t* src_rect, khandle_t dst, krect_t* dst_rect)
{
    kcall4(SYSCALL_SURF_BLIT, src, (uint32_t)src_rect, dst, (uint32_t)dst_rect);
}
void KSVC_DLL ksurf_blit_srckeyed(khandle_t src, const krect_t* src_rect, khandle_t dst, krect_t* dst_rect, uint16_t srckey_color)
{
    kcall5(SYSCALL_SURF_BLIT_SRCKEYED, src, (uint32_t)src_rect, dst, (uint32_t)dst_rect, srckey_color);
}
void KSVC_DLL ksurf_fill(khandle_t dst, const krect_t* dst_rect, uint16_t color)
{
    kcall3(SYSCALL_SURF_FILL, dst, (uint32_t)dst_rect, color);
}
khandle_t KSVC_DLL ksurf_get_primary()
{
    kcall0(SYSCALL_SURF_GET_PRIMARY);
}

khandle_t KSVC_DLL kthread_create(kthread_entry_t entry, void* param, uint32_t stack_size, uint32_t suspended)
{
    return kcall4(SYSCALL_THREAD_CREATE, (uint32_t)entry, (uint32_t)param, stack_size, suspended);
}
int32_t KSVC_DLL kthread_suspend(khandle_t thread)
{
    return kcall1(SYSCALL_THREAD_SUSPEND, thread);
}
int32_t KSVC_DLL kthread_resume(khandle_t thread)
{
    return kcall1(SYSCALL_THREAD_RESUME, thread);
}
void KSVC_DLL kthread_leave()
{
    kcall0(SYSCALL_THREAD_LEAVE);
}
void KSVC_DLL kthread_wait(khandle_t thread)
{
    kcall1(SYSCALL_THREAD_WAIT, thread);
}
khandle_t KSVC_DLL kthread_get_current_thread()
{
    return kcall0(SYSCALL_THREAD_GET_CURRENT_THREAD);
}
uint32_t KSVC_DLL kthread_get_id(khandle_t thread)
{
    return kcall1(SYSCALL_THREAD_GET_ID, thread);
}

khandle_t KSVC_DLL kwave_open(uint32_t sample_rate, uint32_t bits_per_sample)
{
    return kcall2(SYSCALL_WAVE_OPEN, sample_rate, bits_per_sample);
}
void KSVC_DLL kwave_reset(khandle_t handle) {
    kcall1(SYSCALL_WAVE_RESET, handle);
}
uint32_t KSVC_DLL kwave_wait_message(khandle_t handle)
{
    return kcall1(SYSCALL_WAVE_WAIT_MESSAGE, handle);
}
void KSVC_DLL kwave_add_buffer(khandle_t handle, void* buffer_data, uint32_t size)
{
    kcall3(SYSCALL_WAVE_ADD_BUFFER, handle, (uint32_t)buffer_data, size);
}


uint32_t KSVC_DLL win32_get_module_handle(const char* module_name)
{
    return kcall1(SYSCALL_WIN32_GET_MODULE_HANDLE, (uint32_t)module_name);
}
uint32_t KSVC_DLL win32_get_module_file_name(uint32_t handle, char* buffer, uint32_t size)
{
    return kcall3(SYSCALL_WIN32_GET_MODULE_FILE_NAME, handle, (uint32_t)buffer, size);
}
void* KSVC_DLL win32_get_proc_address(uint32_t handle, const char* name)
{
    return (void*)kcall2(SYSCALL_WIN32_GET_PROC_ADDRESS, handle, (uint32_t)name);
}
uint32_t KSVC_DLL win32_load_library(const char* name) {
    return kcall1(SYSCALL_WIN32_LOAD_LIBRARY, (uint32_t)name);
}
void* KSVC_DLL win32_get_entry_point(uint32_t handle) {
    return (void*)kcall1(SYSCALL_WIN32_GET_ENTRY_POINT, handle);
}

void KSVC_DLL win32_mq_subscribe_global(void)
{
    kcall0(SYSCALL_WIN32_MQ_SUBSCRIBE_GLOBAL);
}
void KSVC_DLL win32_mq_post(khandle_t thread, uint32_t hwnd, uint32_t message, uint32_t wparam, uint32_t lparam)
{
    kcall5(SYSCALL_WIN32_MQ_POST, thread, hwnd, message, wparam, lparam);
}
/*uint32_t KSVC_DLL win32_mq_send(khandle_t thread, uint32_t hwnd, uint32_t message, uint32_t wparam, uint32_t lparam)
{
    return kcall5(SYSCALL_WIN32_MQ_SEND, thread, hwnd, message, wparam, lparam);
}*/
int32_t KSVC_DLL win32_mq_try_pop(win32_mq_msg_t* out_msg)
{
    return kcall1(SYSCALL_WIN32_MQ_TRY_POP, (uint32_t)out_msg);
}
void KSVC_DLL win32_mq_pop(win32_mq_msg_t* out_msg)
{
    kcall1(SYSCALL_WIN32_MQ_POP, (uint32_t)out_msg);
}
/*void KSVC_DLL win32_mq_reply(uint32_t reply)
{
    kcall1(SYSCALL_WIN32_MQ_REPLY, reply);
}*/


uint32_t KSVC_DLL win32_timer_create(khandle_t thread, uint32_t hwnd, uint32_t interval_ms, void* func)
{
    return kcall4(SYSCALL_WIN32_TIMER_CREATE, thread, hwnd, interval_ms, (uint32_t)func);
}
void KSVC_DLL win32_timer_set_interval(uint32_t timer_id, uint32_t interval_ms)
{
    kcall2(SYSCALL_WIN32_TIMER_SET_INTERVAL, timer_id, interval_ms);
}
void KSVC_DLL win32_kill_timer(uint32_t timer_id)
{
    kcall1(SYSCALL_WIN32_TIMER_KILL, timer_id);
}
