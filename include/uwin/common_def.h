
/* definitions shared with KSVC.DLL */

#ifndef COMMON_DEF_H
#define COMMON_DEF_H

#include <stdint.h>

#define SYSCALL_PANIC                       0x0001
#define SYSCALL_EXIT                        0x0002
#define SYSCALL_PRINT                       0x0003

#define SYSCALL_MAP_MEMORY_DYNAMIC          0x0101
#define SYSCALL_MAP_MEMORY_FIXED            0x0102
#define SYSCALL_UNMAP_MEMORY                0x0103
#define SYSCALL_PROTECT_MEMORY              0x0104

#define SYSCALL_GET_MONOTONIC_TIME          0x0201
#define SYSCALL_GET_REAL_TIME               0x0202
#define SYSCALL_GET_TIMEZONE_OFFSET         0x0203
#define SYSCALL_SLEEP                       0x0204

#define SYSCALL_HT_DUMMY_NEW                0x0301
#define SYSCALL_HT_DUMMY_GET                0x0302
#define SYSCALL_HT_DUMMY_GETTYPE            0x0303
#define SYSCALL_HT_NEWREF                   0x0304
#define SYSCALL_HT_DELREF                   0x0305
#define SYSCALL_HT_GETTYPE                  0x0306

#define SYSCALL_SEM_ALLOC                   0x0401
#define SYSCALL_SEM_POST                    0x0403
#define SYSCALL_SEM_WAIT                    0x0404

#define SYSCALL_MUT_ALLOC                   0x0411
#define SYSCALL_MUT_LOCK                    0x0412
#define SYSCALL_MUT_UNLOCK                  0x0413

#define SYSCALL_COND_ALLOC                  0x0421
#define SYSCALL_COND_SIGNAL                 0x0422
#define SYSCALL_COND_BROADCAST              0x0423
#define SYSCALL_COND_WAIT                   0x0424

#define SYSCALL_CFG_WRITE                   0x0501
#define SYSCALL_CFG_READ                    0x0502
#define SYSCALL_CFG_COMMIT                  0x0503

#define SYSCALL_FILE_OPEN                   0x0601
#define SYSCALL_FILE_READ                   0x0602
#define SYSCALL_FILE_WRITE                  0x0603
#define SYSCALL_FILE_SEEK                   0x0604
#define SYSCALL_FILE_TELL                   0x0605
#define SYSCALL_FILE_GET_FREE_SPACE         0x0606
#define SYSCALL_FILE_SET_CURRENT_DIRECTORY  0x0607
#define SYSCALL_FILE_GET_CURRENT_DIRECTORY  0x0608
#define SYSCALL_FILE_STAT                   0x0609
#define SYSCALL_FILE_FSTAT                  0x060a
#define SYSCALL_FILE_OPENDIR                0x060b
#define SYSCALL_FILE_READDIR                0x060c

#define SYSCALL_SURF_ALLOC                  0x0701
#define SYSCALL_SURF_LOCK                   0x0702
#define SYSCALL_SURF_UNLOCK                 0x0703
#define SYSCALL_SURF_BLIT                   0x0704
#define SYSCALL_SURF_BLIT_SRCKEYED          0x0705
#define SYSCALL_SURF_FILL                   0x0706
#define SYSCALL_SURF_GET_PRIMARY            0x0707

#define SYSCALL_THREAD_CREATE               0x0801
#define SYSCALL_THREAD_SUSPEND              0x0802
#define SYSCALL_THREAD_RESUME               0x0803
#define SYSCALL_THREAD_LEAVE                0x0804
#define SYSCALL_THREAD_WAIT                 0x0805
#define SYSCALL_THREAD_GET_CURRENT_THREAD   0x0806
#define SYSCALL_THREAD_GET_ID               0x0807

#define SYSCALL_WAVE_OPEN                   0x0901
#define SYSCALL_WAVE_RESET                  0x0902
#define SYSCALL_WAVE_WAIT_MESSAGE           0x0903
#define SYSCALL_WAVE_ADD_BUFFER             0x0904

#define SYSCALL_WIN32_GET_MODULE_FILE_NAME      0x1001
#define SYSCALL_WIN32_GET_MODULE_HANDLE         0x1002
#define SYSCALL_WIN32_GET_PROC_ADDRESS          0x1003
#define SYSCALL_WIN32_LOAD_LIBRARY              0x1004
#define SYSCALL_WIN32_GET_ENTRY_POINT           0x1005

#define SYSCALL_WIN32_MQ_SUBSCRIBE_GLOBAL       0x1101
#define SYSCALL_WIN32_MQ_POST                   0x1102
#define SYSCALL_WIN32_MQ_TRY_POP                0x1103
#define SYSCALL_WIN32_MQ_POP                    0x1104

#define SYSCALL_WIN32_TIMER_CREATE              0x1201
#define SYSCALL_WIN32_TIMER_SET_INTERVAL        0x1202
#define SYSCALL_WIN32_TIMER_KILL                0x1203

#define UW_OBJ_DUMMY       0x0
#define UW_OBJ_SEM         0x1
#define UW_OBJ_MUT         0x2
#define UW_OBJ_COND        0x3
#define UW_OBJ_FILE        0x4
#define UW_OBJ_SURF        0x5
#define UW_OBJ_WAVE        0x6
#define UW_OBJ_DIR         0x7
#define UW_OBJ_THREAD      0x8

#define UW_FILE_READ 0x1
#define UW_FILE_WRITE 0x2
#define UW_FILE_CREATE 0x4
#define UW_FILE_TRUNCATE 0x8
#define UW_FILE_APPEND 0x10

#define UW_FILE_SEEK_BGN 0x0
#define UW_FILE_SEEK_CUR 0x1
#define UW_FILE_SEEK_END 0x2

#define UW_TEB_SEH_FRAME        0x0
#define UW_TEB_STACK_BOTTOM     0x1
#define UW_TEB_STACK_TOP        0x2
#define UW_TEB_PTEB             0x6 // address of teb itself
#define UW_TEB_INITLIST         0x7 // not same as windows
#define UW_TEB_PID              0x8
#define UW_TEB_TID              0x9
#define UW_TEB_CMDLINE          0xa // not same as windows
#define UW_TEB_GDI_BATCH_LIMIT  0xb // not same as windows
#define UW_TEB_LAST_ERROR       0xd

#define UW_WAVE_DONE        0x1
#define UW_WAVE_CLOSE       0x2
#define UW_WAVE_RESET       0x3
#define UW_WAVE_ENQUEUED    0x4

#define UW_INITIAL_THREAD_ID 2

#define UW_GUEST_PROG_PATH "C:\\HOMM3"

typedef struct uw_rect {
    int32_t left;
    int32_t top;
    int32_t right;
    int32_t bottom;
} __attribute__((packed)) uw_rect_t;


#define UW_FILE_MODE_FILE 0x1
#define UW_FILE_MODE_DIR 0x2

typedef struct uw_file_stat_t {
    uint32_t mode;
    uint64_t size;
    uint64_t atime;
    uint64_t mtime;
    uint64_t ctime;
} __attribute__((packed)) uw_file_stat_t;

#endif
