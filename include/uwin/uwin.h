#ifndef QEMU_H
#define QEMU_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
//#include <windows.h>
//#include <winternl.h>

#include "target_abi.h"
#include "common_def.h"
#include "uwindows.h"
#include "uwin/util/log.h"
#include "mem.h"

#include <pthread.h>

//#define UW_HOST_PROG_PATH "../homm3"
#define UW_CONFIG_PATH "homm3.cfg"
#define UW_TARGET_CONFIG_GROUP "homm3_config"


// assert that we have little endian architecture
#if defined(__BYTE_ORDER) && __BYTE_ORDER == __BIG_ENDIAN || \
    defined(__BIG_ENDIAN__) || \
    defined(__ARMEB__) || \
    defined(__THUMBEB__) || \
    defined(__AARCH64EB__) || \
    defined(_MIBSEB) || defined(__MIBSEB) || defined(__MIBSEB__)
#error "Big endian architectires are not supported"
#elif defined(__BYTE_ORDER) && __BYTE_ORDER == __LITTLE_ENDIAN || \
    defined(__LITTLE_ENDIAN__) || \
    defined(__ARMEL__) || \
    defined(__THUMBEL__) || \
    defined(__AARCH64EL__) || \
    defined(_MIPSEL) || defined(__MIPSEL) || defined(__MIPSEL__)
// It's a little-endian target architecture
#else
#error "I don't know what architecture this is!"
#endif


typedef struct uw_win32_mq uw_win32_mq_t;
typedef struct uw_thread uw_thread_t;

typedef struct uw_target_process_data {
    uint32_t process_id;
    uint32_t hmodule;
    uint32_t gdt_base;
    uint32_t idt_base;
    
    uint32_t library_init_list;
    uint32_t command_line;
    uint32_t init_entry;
} uw_target_process_data_t;

typedef struct uw_target_thread_data {
    uint32_t thread_id;
    uw_thread_t* thread_obj;
    uint32_t entry_point;
    uint32_t entry_param;
    
    uint32_t stack_top;
    uint32_t stack_size;
    
    uint32_t tls_base;
    uint32_t gdt_base;
    uint32_t teb_base;
    
    // used to implement suspend
    volatile int32_t suspend_request_count;
    volatile int suspended;
    pthread_cond_t suspend_cond;
    pthread_mutex_t suspend_mutex;
    
    uw_target_process_data_t* process;
    
    uw_win32_mq_t* win32_mq;
    volatile int running;
    uint32_t self_handle;
} uw_target_thread_data_t;

void* ldr_load_executable_and_prepare_initial_thread(const char* exec_path, uw_target_process_data_t *process_data);

uint32_t ldr_get_module_filename(uint32_t module_handle, char* buffer, int buffer_size);
uint32_t ldr_get_module_handle(const char* module_name);
uint32_t ldr_get_proc_address(uint32_t module_handle, const char* proc_name);
char* ldr_beautify_address(uint32_t addr);
void ldr_write_gdb_setup_script(int port, const char* debug_path, FILE* f);
void ldr_print_memory_map(void);
uint32_t ldr_load_library(const char* module_name);
uint32_t ldr_get_entry_point(uint32_t module_handle);

extern __thread uint32_t* win32_last_error;

#define win32_err (*win32_last_error)

/* setup platform-dependent fault handler (used by tcg) */
void setup_fault_handler(void);

// TODO: rework this api
// Why would one want to make the target memory executable anyway?
// These should be emulated access modes, not host values
#define UW_PROT_R   0
#define UW_PROT_RW  1
#define UW_PROT_RX  2
#define UW_PROT_N   3
//#define UW_PROT_RWX 4



// (somewhat) platform-independent API for guest memory mapping

/* initializes mappings. returns a guest_base */
void* uw_memory_map_initialize(void);
/* uninitializes mappings */
void uw_memory_map_finalize(void);
/* map a physical memory to a fixed location. size MUST be page-aligned */
int uw_target_map_memory_fixed(uint32_t address, uint32_t size, int prot);
/* map a physical memory _somewhere_. size MUST be page-aligned */
uint32_t uw_target_map_memory_dynamic(uint32_t size, int prot);
/* unmap physical memory from specified range. size MUST be page-aligned */
int uw_unmap_memory(uint32_t address, uint32_t size);
/* changes protection of memory range */
int uw_set_memprotect(uint32_t address, uint32_t size, int prot);

extern uint32_t uw_host_page_size;

// designed for big chunks of memory; size should be aligned at least to uw_host_page_size
int uw_jitmem_alloc(void** rw_addr, void** exec_addr, size_t size);
void uw_jitmem_free(void* rw_addr, void* exec_addr, size_t size);

int32_t do_syscall(int num, uint32_t arg1,
                    uint32_t arg2, uint32_t arg3, uint32_t arg4,
                    uint32_t arg5, uint32_t arg6, uint32_t arg7,
                    uint32_t arg8);

// uw_cpu_loop stuff
void* uw_cpu_alloc_context();
void uw_cpu_free_context(void* cpu_context);
void uw_cpu_panic(const char* message);
void uw_cpu_setup_thread(void* cpu_context, uw_target_thread_data_t *params);

void uw_cpu_loop(void* cpu_context);

uint64_t uw_get_time_us(void);  // TODO: migrate from this function
uint64_t uw_get_monotonic_time(void);
uint64_t uw_get_real_time(void);
uint32_t uw_get_timezone_offset(void);
void uw_sleep(uint32_t time_ms);

typedef struct uw_sem uw_sem_t;

// semaphore library
void uw_sem_initialize(void);
void uw_sem_finalize(void);
uw_sem_t* uw_sem_alloc(int count);
void uw_sem_destroy(uw_sem_t* sem);
void uw_sem_post(uw_sem_t* sem);
int32_t uw_sem_wait(uw_sem_t* sem, uint64_t tmout_us);

typedef struct uw_mut uw_mut_t;

// mutex library
void uw_mut_initialize(void);
void uw_mut_finalize(void);
uw_mut_t* uw_mut_alloc();
void uw_mut_destroy(uw_mut_t* mut);
int32_t uw_mut_lock(uw_mut_t* mut, uint64_t tmout_us);
void uw_mut_unlock(uw_mut_t* mut);

typedef struct uw_cond uw_cond_t;

// conditional variable library
void uw_cond_initialize(void);
void uw_cond_finalize(void);
uw_cond_t* uw_cond_alloc();
void uw_cond_destroy(uw_cond_t* cond);
void uw_cond_signal(uw_cond_t* cond);
void uw_cond_broadcast(uw_cond_t* cond);
int32_t uw_cond_wait(uw_cond_t* cond, uw_mut_t* mut, uint64_t tmout_us);

// handle table library
void uw_ht_initialize(void);
void uw_ht_finalize(void);
uint32_t uw_ht_put(void* obj, int type);
uint32_t uw_ht_put_dummy(uint32_t obj, int type);
uint32_t uw_ht_get_dummy(uint32_t handle, int type);
int uw_ht_get_dummytype(uint32_t handle);
void* uw_ht_get(uint32_t handle, int type);
uint32_t uw_ht_newref(uint32_t handle);
void uw_ht_delref(uint32_t handle, uint32_t* dummy_type, uint32_t* dummy_value);
int uw_ht_gettype(uint32_t handle);

// config library
void uw_cfg_initialize(void);
void uw_cfg_finalize(void);
void uw_cfg_write(const char* key, const char* value);
int32_t uw_cfg_read(const char* key, char* buffer, uint32_t size);
void uw_cfg_commit(void);

typedef struct uw_file uw_file_t;
typedef struct uw_dir uw_dir_t;

// file helpers. Paths are from win32
void uw_file_initialize(void);
void uw_file_finalize(void);
void uw_file_set_host_directory(const char* path);
uw_file_t* uw_file_open(const char* file_name, int mode);
int32_t uw_file_read(uw_file_t* file, char* buffer, uint32_t length);
int32_t uw_file_write(uw_file_t* file, const char* buffer, uint32_t length);
int32_t uw_file_seek(uw_file_t* file, int64_t seek, uint32_t origin);
int64_t uw_file_tell(uw_file_t* file);
void uw_file_close(uw_file_t* file);
void uw_file_get_free_space(uint64_t* free_bytes, uint64_t* total_bytes);
int32_t uw_file_set_current_directory(const char* new_current_directory);
void uw_file_get_current_directory(char* buffer, uint32_t size);
void uw_file_stat(const char* file_name, uw_file_stat_t* stat);
void uw_file_fstat(uw_file_t* file, uw_file_stat_t* stat);
uw_dir_t* uw_file_opendir(const char* file_name);
int32_t uw_file_readdir(uw_dir_t* dir, char* buffer, uint32_t size);
void uw_file_closedir(uw_dir_t* dir);

typedef struct uw_surf uw_surf_t;
typedef struct uw_wave uw_wave_t;

typedef struct uw_locked_surf_desc {
    void* data;
    uint32_t pitch;
    uint32_t w, h;
} uw_locked_surf_desc_t;

typedef struct target_uw_locked_surf_desc {
    uint32_t data;
    uint32_t pitch;
    uint32_t w, h;
} __attribute__((packed)) target_uw_locked_surf_desc_t;

// ui libraries: gfx (surface manipulation), input and wave
// all pixel formats are RGB565
void uw_ui_initialize(void);
void uw_ui_finalize(void);

uw_surf_t* uw_ui_surf_alloc(int32_t width, int32_t height);
void uw_ui_surf_free(uw_surf_t* surf);
void uw_ui_surf_lock(uw_surf_t* surf, uw_locked_surf_desc_t* locked);
void uw_ui_surf_unlock(uw_surf_t* surf);
void uw_ui_surf_blit(uw_surf_t* src, const uw_rect_t* src_rect, uw_surf_t* dst, uw_rect_t* dst_rect);
void uw_ui_surf_blit_srckeyed(uw_surf_t* src, const uw_rect_t* src_rect, uw_surf_t* dst, uw_rect_t* dst_rect, uint16_t srckey_color);
void uw_ui_surf_fill(uw_surf_t* dst, uw_rect_t* dst_rect, uint16_t color);
uw_surf_t* uw_ui_surf_get_primary(void); // should be called only once

uw_wave_t* uw_ui_wave_open(uint32_t sample_rate, uint32_t bits_per_sample);
void uw_ui_wave_close(uw_wave_t* wave);
void uw_ui_wave_reset(uw_wave_t* wave);
uint32_t uw_ui_wave_wait_message(uw_wave_t* wave);
void uw_ui_wave_add_buffer(uw_wave_t* wave, void* buffer, uint32_t size);

// sighandler
void uw_sighandler_initialize(void);
void uw_sighandler_finalize(void);

// guest thread management
void uw_thread_initialize(void);
void uw_thread_finalize(void);
void* uw_create_initial_thread(uw_target_process_data_t *process_data, uint32_t entry, uint32_t entry_param, uint32_t stack_size);
void uw_start_initial_thread(void* initial_thread_param);
uint32_t uw_thread_create(uint32_t entry, uint32_t entry_param, uint32_t stack_size, uint32_t suspended); // unlike other APIs, this one returns a handle
int32_t uw_thread_suspend(uw_thread_t* thread);
int32_t uw_thread_resume(uw_thread_t* thread);
void uw_thread_leave(void); // leave the current thread
uw_target_thread_data_t* uw_thread_get_data(uw_thread_t* thread);
//pthread_mutex_t* uw_thread_get_table_mutex(void);
void uw_thread_wait(uw_thread_t* thread);
void uw_thread_destroy(uw_thread_t* thread);

typedef struct uw_gmq_msg {
    int32_t message;
    void* param1;
    void* param2;
} uw_gmq_msg_t;

#define UW_GM_QUIT              0x1
#define UW_GM_KEYDOWN           0x2 // win32 virtual key codes are used for gmq keyboard events. Mouse events are separate
#define UW_GM_KEYUP             0x3

#define UW_GM_MOUSELDOWN        0x4 // correspont to winapi mouse messages.
#define UW_GM_MOUSELUP          0x5
#define UW_GM_MOUSELDOUBLECLICK 0x6
#define UW_GM_MOUSERDOWN        0x7
#define UW_GM_MOUSERUP          0x8
#define UW_GM_MOUSERDOUBLECLICK 0x9
#define UW_GM_MOUSEMOVE         0xa

#define UW_GM_NEWTHREAD         0xb
#define UW_GM_THREAD_DIED       0xc

// global message queue
void uw_gmq_initialize(void);
void uw_gmq_finalize(void);
void uw_gmq_post(int32_t message, void* param1, void* param2);
void uw_gmq_poll(uw_gmq_msg_t* result_message); // should be called only by main thread

typedef struct uw_win32_mq_msg {
    uint32_t hwnd;
    uint32_t message;
    uint32_t wparam;
    uint32_t lparam;
    uint32_t time; // which time?
    uint32_t pt_x;
    uint32_t pt_y;
} __attribute__((packed)) uw_win32_mq_msg_t;

// win32 thread-local message queue
void uw_win32_mq_initialize(void);
void uw_win32_mq_finalize(void);
void uw_win32_mq_subscribe_global(void); // subscribe to input events
void uw_win32_mq_publish_global_message(uw_gmq_msg_t* message);
uw_win32_mq_t* uw_win32_mq_create(void);
void uw_win32_mq_free(uw_win32_mq_t* mq);
void uw_win32_mq_post(uw_thread_t* thread, uint32_t hwnd, uint32_t message, uint32_t wparam, uint32_t lparam);
//uint32_t uw_win32_mq_send(uw_thread_id_t thread_id, uint32_t hwnd, uint32_t message, uint32_t wparam, uint32_t lparam);
int32_t uw_win32_mq_try_pop(uw_win32_mq_msg_t* out_message);
void uw_win32_mq_pop(uw_win32_mq_msg_t* out_message);
//void uw_win32_mq_reply(uint32_t reply);

/*
void uw_win32_timer_initialize(void);
void uw_win32_timer_finalize(void);
uint32_t uw_win32_timer_create(uw_thread_t* thread, uint32_t hwnd, uint32_t interval_ms, uint32_t func);
void uw_win32_timer_set_interval(uint32_t timer_id, uint32_t interval_ms);
void uw_win32_kill_timer(uint32_t timer_id);
*/

#define uw_ht_get_sem(handle) ((uw_sem_t*)uw_ht_get(handle, UW_OBJ_SEM))
#define uw_ht_get_mut(handle) ((uw_mut_t*)uw_ht_get(handle, UW_OBJ_MUT))
#define uw_ht_get_cond(handle) ((uw_cond_t*)uw_ht_get(handle, UW_OBJ_COND))
#define uw_ht_get_file(handle) ((uw_file_t*)uw_ht_get(handle, UW_OBJ_FILE))
#define uw_ht_get_surf(handle) ((uw_surf_t*)uw_ht_get(handle, UW_OBJ_SURF))
#define uw_ht_get_wave(handle) ((uw_wave_t*)uw_ht_get(handle, UW_OBJ_WAVE))
#define uw_ht_get_dir(handle) ((uw_dir_t*)uw_ht_get(handle, UW_OBJ_DIR))
#define uw_ht_get_thread(handle) ((uw_thread_t*)uw_ht_get(handle, UW_OBJ_THREAD))

#define uw_current_thread_id   (uw_current_thread_data->thread_id)
#define uw_current_thread      (uw_current_thread_data->thread_obj)


#define VERIFY_READ 0
#define VERIFY_WRITE 1 /* implies read access */


extern __thread uw_target_thread_data_t* uw_current_thread_data;
//extern abi_ulong target_idt_base;
//extern abi_ulong target_gdt_base;

#ifdef __cplusplus
};
#endif

#endif /* QEMU_H */
