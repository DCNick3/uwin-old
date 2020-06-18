
#ifndef KSVC_H
#define KSVC_H

#ifdef BUILDING_KSVC
#define KSVC_DLL __declspec(dllexport)
#else
#define KSVC_DLL __declspec(dllimport)
#endif

#include "common_def.h"
#include <stdint.h>

int32_t KSVC_DLL kcall0(uint32_t knum);
int32_t KSVC_DLL kcall1(uint32_t knum, uint32_t arg0);
int32_t KSVC_DLL kcall2(uint32_t knum, uint32_t arg0, uint32_t arg1);
int32_t KSVC_DLL kcall3(uint32_t knum, uint32_t arg0, uint32_t arg1, uint32_t arg2);
int32_t KSVC_DLL kcall4(uint32_t knum, uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3);
int32_t KSVC_DLL kcall5(uint32_t knum, uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4);
int32_t KSVC_DLL kcall6(uint32_t knum, uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t arg5); 

void KSVC_DLL kexit(void) __attribute__((noreturn));
void KSVC_DLL kpanic(const char* message) __attribute__((noreturn));
void KSVC_DLL kpanicf(const char *format, ...) __attribute__((noreturn));
void KSVC_DLL kprint(const char* message);
int KSVC_DLL vkprintf(const char* format, va_list va);
int KSVC_DLL kprintf(const char* format, ...);

#define PROT_R  0
#define PROT_RW 1
#define PROT_RX 2
#define PROT_N  3

typedef uw_rect_t krect_t;

typedef struct ksurf_lockdesc_t {
    void* data;
    uint32_t pitch;
    uint32_t w, h;
} ksurf_lockdesc_t;

typedef uint32_t khandle_t;

typedef uint32_t (__stdcall *kthread_entry_t)(void* param);
//typedef uint32_t kthread_id_t;
typedef uw_file_stat_t kfile_stat_t;

void*        KSVC_DLL kmap_memory_dynamic(uint32_t size, uint32_t prot);
uint32_t     KSVC_DLL kmap_memory_fixed(void* address, uint32_t size, uint32_t prot);
int32_t      KSVC_DLL kunmap_memory(void* address, uint32_t size);
int32_t      KSVC_DLL kprotect_memory(void* address, uint32_t size, uint32_t prot);

uint64_t     KSVC_DLL kget_monotonic_time();
uint64_t     KSVC_DLL kget_real_time();
uint32_t     KSVC_DLL kget_timezone_offset();
void         KSVC_DLL ksleep(uint32_t time_ms);

khandle_t    KSVC_DLL kht_dummy_new(uint32_t type, uint32_t value);
uint32_t     KSVC_DLL kht_dummy_get(khandle_t handle, uint32_t type);
uint32_t     KSVC_DLL kht_dummy_gettype(khandle_t handle);
khandle_t    KSVC_DLL kht_newref(khandle_t handle);
void         KSVC_DLL kht_delref(khandle_t handle, uint32_t* dummy_type, uint32_t* dummy_value);
uint32_t     KSVC_DLL kht_gettype(khandle_t handle);

khandle_t    KSVC_DLL ksem_alloc(uint32_t value);
//void         KSVC_DLL ksem_destroy(khandle_t handle);
void         KSVC_DLL ksem_post(khandle_t handle);
int32_t      KSVC_DLL ksem_wait(khandle_t handle, uint64_t timeout_us);

khandle_t    KSVC_DLL kmut_alloc();
int32_t      KSVC_DLL kmut_lock(khandle_t handle, uint64_t timeout_us);
void         KSVC_DLL kmut_unlock(khandle_t handle);

khandle_t    KSVC_DLL kcond_alloc();
void         KSVC_DLL kcond_signal(khandle_t handle);
void         KSVC_DLL kcond_broadcast(khandle_t handle);
int32_t      KSVC_DLL kcond_wait(khandle_t cond_handle, khandle_t mutex_handle, uint64_t timeout_us);

int32_t      KSVC_DLL kcfg_read(const char* key, char* buffer, uint32_t buffer_size);
char*        KSVC_DLL kcfg_read_all(const char* key);
void         KSVC_DLL kcfg_write(const char* key, const char* value);
void         KSVC_DLL kcfg_commit();

khandle_t    KSVC_DLL kfile_open(const char* name, uint32_t mode);
int32_t      KSVC_DLL kfile_read(khandle_t handle, char* buffer, uint32_t buffer_size);
int32_t      KSVC_DLL kfile_write(khandle_t handle, const char* buffer, uint32_t buffer_size);
int32_t      KSVC_DLL kfile_seek(khandle_t handle, int64_t seek, uint32_t origin);
int64_t      KSVC_DLL kfile_tell(khandle_t handle);
void         KSVC_DLL kfile_get_free_space(uint64_t *free_bytes, uint64_t *total_bytes);
int32_t      KSVC_DLL kfile_set_current_directory(const char *new_current_directory);
void         KSVC_DLL kfile_get_current_directory(char *buffer, uint32_t size);
void         KSVC_DLL kfile_stat(const char* path, kfile_stat_t* stat);
void         KSVC_DLL kfile_fstat(khandle_t file, kfile_stat_t* stat);
khandle_t    KSVC_DLL kfile_opendir(const char* path);
int32_t      KSVC_DLL kfile_readdir(khandle_t dir, char* buffer, uint32_t size);

khandle_t    KSVC_DLL ksurf_alloc(int32_t width, uint32_t height);
void         KSVC_DLL ksurf_lock(khandle_t handle, ksurf_lockdesc_t* lockdesc);
void         KSVC_DLL ksurf_unlock(khandle_t handle);
void         KSVC_DLL ksurf_blit(khandle_t src, const krect_t* src_rect, khandle_t dst, krect_t* dst_rect);
void         KSVC_DLL ksurf_blit_srckeyed(khandle_t src, const krect_t* src_rect, khandle_t dst, krect_t* dst_rect, uint16_t srckey_color);
void         KSVC_DLL ksurf_fill(khandle_t dst, const krect_t* dst_rect, uint16_t color);
khandle_t    KSVC_DLL ksurf_get_primary();

khandle_t    KSVC_DLL kthread_create(kthread_entry_t entry, void* param, uint32_t stack_size, uint32_t suspended);
int32_t      KSVC_DLL kthread_suspend(khandle_t thread);
int32_t      KSVC_DLL kthread_resume(khandle_t thread);
void         KSVC_DLL kthread_leave();
void         KSVC_DLL kthread_wait(khandle_t thread);
khandle_t    KSVC_DLL kthread_get_current_thread(); // returns a self-handle, used to keep thread object alive during its execution. DO NOT CLOSE IT
uint32_t     KSVC_DLL kthread_get_id(khandle_t thread);

khandle_t    KSVC_DLL kwave_open(uint32_t sample_rate, uint32_t bits_per_sample);
void         KSVC_DLL kwave_reset(khandle_t handle);
uint32_t     KSVC_DLL kwave_wait_message(khandle_t handle);
void         KSVC_DLL kwave_add_buffer(khandle_t handle, void* buffer_data, uint32_t size);

#define KFILE_READ UW_FILE_READ
#define KFILE_WRITE UW_FILE_WRITE
#define KFILE_CREATE UW_FILE_CREATE
#define KFILE_TRUNCATE UW_FILE_TRUNCATE
#define KFILE_APPEND UW_FILE_APPEND

#define KFILE_BGN UW_FILE_SEEK_BGN
#define KFILE_CUR UW_FILE_SEEK_CUR
#define KFILE_END UW_FILE_SEEK_END

#define KFILE_MODE_FILE UW_FILE_MODE_FILE
#define KFILE_MODE_DIR UW_FILE_MODE_DIR

#define KHT_DUMMY   UW_OBJ_DUMMY
#define KHT_SEM     UW_OBJ_SEM
#define KHT_FILE    UW_OBJ_FILE
#define KHT_THREAD  UW_OBJ_THREAD

#define TEB_SEH_FRAME       (4*UW_TEB_SEH_FRAME)
#define TEB_STACK_BOTTOM    (4*UW_TEB_STACK_BOTTOM)
#define TEB_STACK_TOP       (4*UW_TEB_STACK_TOP)
#define TEB_PTEB            (4*UW_TEB_PTEB)
#define TEB_INITLIST        (4*UW_TEB_INITLIST)
#define TEB_PID             (4*UW_TEB_PID)
#define TEB_TID             (4*UW_TEB_TID)
#define TEB_CMDLINE         (4*UW_TEB_CMDLINE)
#define TEB_GDI_BATCH_LIMIT (4*UW_TEB_GDI_BATCH_LIMIT)
#define TEB_LAST_ERROR      (4*UW_TEB_LAST_ERROR)

#define KTHREAD_INIT_ID UW_INITIAL_THREAD_ID

#define KSEM_TMO_INF 0xffffffffffffffffULL

#define KHT_DUMMY_CONIO             0x1
#define KHT_DUMMY_EVENT             0x2
#define KHT_DUMMY_DEVICE_CONTEXT    0x3
#define KHT_DUMMY_REG_KEY           0x4
#define KHT_DUMMY_MENU              0x5
#define KHT_DUMMY_ICON              0x6
#define KHT_DUMMY_CURSOR            0x7
#define KHT_DUMMY_PROCESS           0x8
//#define KHT_DUMMY_THREAD            0x9
#define KHT_DUMMY_MUTEX             0xa


#define KWAVE_DONE      UW_WAVE_DONE
#define KWAVE_CLOSE     UW_WAVE_CLOSE
#define KWAVE_RESET     UW_WAVE_RESET
#define KWAVE_ENQUEUED  UW_WAVE_ENQUEUED

typedef struct win32_mq_msg {
    uint32_t hwnd;
    uint32_t message;
    uint32_t wparam;
    uint32_t lparam;
    uint32_t time; // which time?
    uint32_t pt_x;
    uint32_t pt_y;
} __attribute__((packed)) win32_mq_msg_t;


uint32_t KSVC_DLL win32_get_module_handle(const char* module_name);
uint32_t KSVC_DLL win32_get_module_file_name(uint32_t handle, char* buffer, uint32_t size);
void*    KSVC_DLL win32_get_proc_address(uint32_t handle, const char* name);
uint32_t KSVC_DLL win32_load_library(const char* name);
void*    KSVC_DLL win32_get_entry_point(uint32_t handle);

void     KSVC_DLL win32_mq_subscribe_global(void);
void     KSVC_DLL win32_mq_post(khandle_t thread, uint32_t hwnd, uint32_t message, uint32_t wparam, uint32_t lparam);
//uint32_t KSVC_DLL win32_mq_send(khandle_t thread, uint32_t hwnd, uint32_t message, uint32_t wparam, uint32_t lparam);
int32_t  KSVC_DLL win32_mq_try_pop(win32_mq_msg_t* out_msg);
void     KSVC_DLL win32_mq_pop(win32_mq_msg_t* out_msg);
//void     KSVC_DLL win32_mq_reply(uint32_t reply);

uint32_t KSVC_DLL win32_timer_create(khandle_t thread, uint32_t hwnd, uint32_t interval_ms, void* func);
void     KSVC_DLL win32_timer_set_interval(uint32_t timer_id, uint32_t interval_ms);
void     KSVC_DLL win32_kill_timer(uint32_t timer_id);

void KSVC_DLL *memset(void *s, int c, size_t n);
void KSVC_DLL *memmove(void *dest, const void *src, size_t n);
void KSVC_DLL *memcpy(void *dest, const void *src, size_t n);

char*  KSVC_DLL strdup(const char *s);
size_t KSVC_DLL strlen(const char *s);
int    KSVC_DLL strcmp(const char *s1, const char *s2);
long   KSVC_DLL strtol(const char *restrict nptr, char **restrict endptr, int base);
int    KSVC_DLL isspace(int c);
char*  KSVC_DLL strcat(char* dst, const char* src);
char*  KSVC_DLL strchr(const char *s, int c);
char*  KSVC_DLL strrchr(const char *s, int c);
int    KSVC_DLL tolower(int c);
int    KSVC_DLL stricmp(const char *s1, const char *s2);
#define strcasecmp stricmp

#define TLS_HEAP_OFFSET 4

static inline void* get_tls_value(uint32_t offset)
{
    uint32_t r;
    asm(
        "mov %%gs:(%1), %0"
        : "=r"(r)
        : "r"(offset)
    );
    return (void*)r;
}

static inline void set_tls_value(uint32_t offset, void* value)
{
    asm(
        "mov %0, %%gs:(%1)"
        :
        : "r"(value), "r"(offset)
    );
}

static inline void* get_teb_value(uint32_t offset)
{
    uint32_t r;
    asm(
        "mov %%fs:(%1), %0"
        : "=r"(r)
        : "r"(offset)
    );
    return (void*)r;
}

static inline void set_teb_value(uint32_t offset, void* value)
{
    asm(
        "mov %0, %%fs:(%1)"
        :
        : "r"(value), "r"(offset)
    );
}

#endif
