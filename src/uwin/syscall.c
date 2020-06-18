/*
 *  uwin kernel entry
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



#include "uwin/uwin.h"
#include "uwin/common_def.h"
#include "uwin/util/mem.h"

#include "halfix/cpu/cpu.h"

#include <stdio.h>

#define g2h_n(addr) ( addr == 0 ? NULL : g2h(addr) )

// what was qemu thunk code about, again?)
int32_t do_syscall(int num, uint32_t arg1,
                   uint32_t arg2, uint32_t arg3, uint32_t arg4,
                   uint32_t arg5, uint32_t arg6, uint32_t arg7,
                   uint32_t arg8)
{
    //qemu_log("%02d: syscall %x, %x\n", uw_current_thread_id, (unsigned)num, (unsigned)arg1);
    switch (num) {
        case SYSCALL_PANIC:
            uw_cpu_panic(g2h(arg1)); return 0;
        case SYSCALL_EXIT: uw_log("exit syscall called\n"); exit(0); return 0;
        case SYSCALL_PRINT: uw_log("%02d: %s\n", uw_current_thread_id, (char*)g2h(arg1)); return 0;
        case SYSCALL_MAP_MEMORY_DYNAMIC:
            return uw_target_map_memory_dynamic(arg1, arg2);
        case SYSCALL_MAP_MEMORY_FIXED:
            return uw_target_map_memory_fixed(arg1, arg2, arg3);
        case SYSCALL_UNMAP_MEMORY:
            return uw_unmap_memory(arg1, arg2);
        case SYSCALL_PROTECT_MEMORY:
            return uw_set_memprotect(arg1, arg2, arg3);


        case SYSCALL_GET_MONOTONIC_TIME:
            *(uint64_t*)g2h(arg1) = uw_get_monotonic_time(); return 0;
        case SYSCALL_GET_REAL_TIME:
            *(uint64_t*)g2h(arg1) = uw_get_real_time(); return 0;
        case SYSCALL_GET_TIMEZONE_OFFSET:
            return uw_get_timezone_offset();
        case SYSCALL_SLEEP:
            uw_sleep(arg1); return 0;


        case SYSCALL_HT_DUMMY_NEW:
            return uw_ht_put_dummy(arg2, arg1);
        case SYSCALL_HT_DUMMY_GET:
            return uw_ht_get_dummy(arg1, arg2);
        case SYSCALL_HT_DUMMY_GETTYPE:
            return uw_ht_get_dummytype(arg1);
        case SYSCALL_HT_NEWREF:
            return uw_ht_newref(arg1);
        case SYSCALL_HT_DELREF:
            uw_ht_delref(arg1, g2h_n(arg2), g2h_n(arg3)); return 0;
        case SYSCALL_HT_GETTYPE:
            return uw_ht_gettype(arg1);


        case SYSCALL_SEM_ALLOC:
            return uw_ht_put(uw_sem_alloc(arg1), UW_OBJ_SEM);
        case SYSCALL_SEM_POST:
            uw_sem_post(uw_ht_get_sem(arg1)); return 0;
        case SYSCALL_SEM_WAIT:
            return uw_sem_wait(uw_ht_get_sem(arg1), *(uint64_t *) g2h(arg2));

        case SYSCALL_MUT_ALLOC:
            return uw_ht_put(uw_mut_alloc(), UW_OBJ_MUT);
        case SYSCALL_MUT_LOCK:
            return uw_mut_lock(uw_ht_get_mut(arg1), *(uint64_t*) g2h(arg2));
        case SYSCALL_MUT_UNLOCK:
            uw_mut_unlock(uw_ht_get_mut(arg1)); return 0;

        case SYSCALL_COND_ALLOC:
            return uw_ht_put(uw_cond_alloc(), UW_OBJ_COND);
        case SYSCALL_COND_SIGNAL:
            uw_cond_signal(uw_ht_get_cond(arg1)); return 0;
        case SYSCALL_COND_BROADCAST:
            uw_cond_broadcast(uw_ht_get_cond(arg1)); return 0;
        case SYSCALL_COND_WAIT:
            return uw_cond_wait(uw_ht_get_cond(arg1), uw_ht_get_mut(arg2), *(uint64_t*) g2h(arg3));

        case SYSCALL_CFG_WRITE:
            uw_cfg_write(g2h(arg1), g2h(arg2)); return 0;
        case SYSCALL_CFG_READ:
            return uw_cfg_read(g2h(arg1), g2h(arg2), arg3);
        case SYSCALL_CFG_COMMIT:
            uw_cfg_commit(); return 0;


        case SYSCALL_FILE_OPEN:
            return uw_ht_put(uw_file_open(g2h(arg1), arg2), UW_OBJ_FILE);
        case SYSCALL_FILE_READ:
            return uw_file_read(uw_ht_get_file(arg1), g2h(arg2), arg3);
        case SYSCALL_FILE_WRITE:
            return uw_file_write(uw_ht_get_file(arg1), g2h(arg2), arg3);
        case SYSCALL_FILE_SEEK:
            return uw_file_seek(uw_ht_get_file(arg1), *(int64_t*)g2h(arg2), arg3);
        case SYSCALL_FILE_TELL:
            *(int64_t*)g2h(arg2) = uw_file_tell(uw_ht_get_file(arg1)); return 0;
        case SYSCALL_FILE_GET_FREE_SPACE:
            uw_file_get_free_space(g2h(arg1), g2h(arg2)); return 0;
        case SYSCALL_FILE_SET_CURRENT_DIRECTORY:
            return uw_file_set_current_directory(g2h(arg1));
        case SYSCALL_FILE_GET_CURRENT_DIRECTORY:
            uw_file_get_current_directory(g2h(arg1), arg2); return 0;
        case SYSCALL_FILE_STAT:
            uw_file_stat(g2h(arg1), g2h(arg2)); return 0;
        case SYSCALL_FILE_FSTAT:
            uw_file_fstat(uw_ht_get_file(arg1), g2h(arg2)); return 0;
        case SYSCALL_FILE_OPENDIR:
            return uw_ht_put(uw_file_opendir(g2h(arg1)), UW_OBJ_DIR);
        case SYSCALL_FILE_READDIR:
            return uw_file_readdir(uw_ht_get_dir(arg1), g2h(arg2), arg3);


        case SYSCALL_SURF_ALLOC:
            return uw_ht_put(uw_ui_surf_alloc(arg1, arg2), UW_OBJ_SURF);
        case SYSCALL_SURF_LOCK:
        {
            uw_locked_surf_desc_t host_lockdesc;
            uw_ui_surf_lock(uw_ht_get_surf(arg1), &host_lockdesc);
            target_uw_locked_surf_desc_t* t = g2h(arg2);
            t->data = h2g(host_lockdesc.data);
            t->w = host_lockdesc.w;
            t->h = host_lockdesc.h;
            t->pitch = host_lockdesc.pitch;
            return 0;
        }
        case SYSCALL_SURF_UNLOCK:
            uw_ui_surf_unlock(uw_ht_get_surf(arg1)); return 0;
        case SYSCALL_SURF_BLIT:
            uw_ui_surf_blit(uw_ht_get_surf(arg1), g2h_n(arg2), uw_ht_get_surf(arg3), g2h_n(arg4)); return 0;
        case SYSCALL_SURF_BLIT_SRCKEYED:
            uw_ui_surf_blit_srckeyed(uw_ht_get_surf(arg1), g2h_n(arg2), uw_ht_get_surf(arg3), g2h_n(arg4), arg5); return 0;
        case SYSCALL_SURF_FILL:
            uw_ui_surf_fill(uw_ht_get_surf(arg1), g2h_n(arg2), arg3); return 0;
        case SYSCALL_SURF_GET_PRIMARY:
            return uw_ht_put(uw_ui_surf_get_primary(), UW_OBJ_SURF);


        case SYSCALL_THREAD_CREATE:
            return uw_thread_create(arg1, arg2, arg3, arg4);
        case SYSCALL_THREAD_SUSPEND:
            return uw_thread_suspend(uw_ht_get_thread(arg1));
        case SYSCALL_THREAD_RESUME:
            return uw_thread_resume(uw_ht_get_thread(arg1));
        case SYSCALL_THREAD_LEAVE:
            uw_thread_leave();
            return 0;
        case SYSCALL_THREAD_WAIT:
            uw_thread_wait(uw_ht_get_thread(arg1));
            return 0;
        case SYSCALL_THREAD_GET_CURRENT_THREAD:
            return uw_current_thread_data->self_handle;
        case SYSCALL_THREAD_GET_ID:
            return uw_thread_get_data(uw_ht_get_thread(arg1))->thread_id;
            /*
        case SYSCALL_WAVE_OPEN:
            return uw_ht_put(uw_ui_wave_open(arg1, arg2), UW_OBJ_WAVE);
        case SYSCALL_WAVE_RESET:
            uw_ui_wave_reset(uw_ht_get_wave(arg1)); return 0;
        case SYSCALL_WAVE_WAIT_MESSAGE:
            return uw_ui_wave_wait_message(uw_ht_get_wave(arg1));
        case SYSCALL_WAVE_ADD_BUFFER:
            uw_ui_wave_add_buffer(uw_ht_get_wave(arg1), g2h(arg2), arg3); return 0;
        */
            
        case SYSCALL_WIN32_GET_MODULE_FILE_NAME:            return ldr_get_module_filename(arg1, g2h(arg2), arg3);
        case SYSCALL_WIN32_GET_MODULE_HANDLE:               return ldr_get_module_handle(g2h_n(arg1));
        case SYSCALL_WIN32_GET_PROC_ADDRESS:                return ldr_get_proc_address(arg1, g2h(arg2));
        case SYSCALL_WIN32_LOAD_LIBRARY:                    return ldr_load_library(g2h(arg1));
        case SYSCALL_WIN32_GET_ENTRY_POINT:                 return ldr_get_entry_point(arg1);


        case SYSCALL_WIN32_MQ_SUBSCRIBE_GLOBAL:
            uw_win32_mq_subscribe_global();
            return 0;
        case SYSCALL_WIN32_MQ_POST:
            uw_win32_mq_post(uw_ht_get_thread(arg1), arg2, arg3, arg4, arg5);
            return 0;
        case SYSCALL_WIN32_MQ_TRY_POP:
            return uw_win32_mq_try_pop(g2h(arg1));
        case SYSCALL_WIN32_MQ_POP:
            uw_win32_mq_pop(g2h(arg1));
            return 0;


            /*
            case SYSCALL_WIN32_TIMER_CREATE:
                return uw_win32_timer_create(uw_ht_get_thread(arg1), arg2, arg3, arg4);
            case SYSCALL_WIN32_TIMER_SET_INTERVAL:
                uw_win32_timer_set_interval(arg1, arg2);
                return 0;
            case SYSCALL_WIN32_TIMER_KILL:
                uw_win32_kill_timer(arg1);
                return 0;
                */
        default:
            uw_cpu_panic("Unknown syscall number"); return 0;
    }
}
