
#include <assert.h>
#include <stdint.h>

#include "uwin/uwin.h"
#include "uwin/util/align.h"
#include "uwin/util/mem.h"

#include "halfix/cpuapi.h"
#include "halfix/cpu/cpu.h"
#include "halfix/cpu/libcpu.h"

void* uw_cpu_alloc_context() {
    return uw_new0(struct cpu, 1);
}

void uw_cpu_free_context(void* cpu_context) {
    uw_free(cpu_context);
}

void uw_cpu_panic(const char* message) {
    fprintf(stderr, "PANIC: %s\n\n", message);

    char* bip = ldr_beautify_address(LIN_EIP());
    fprintf(stderr, "%08lx | %s\n", (unsigned long)LIN_EIP(), bip);
    uw_free(bip);

    cpu_debug();

    fprintf(stderr, "Thread id = %d\n", uw_current_thread_id);



    fprintf(stderr, "\nstack dump:\n");
    for (size_t s = 0; s < 0x80; s += 4) {
        uint32_t val = *(uint32_t*)g2h(thread_cpu.reg32[ESP] + s);
        char* baddr = ldr_beautify_address(val);
        fprintf(stderr, "ESP+0x%03zx | 0x%08zx: %08x | %s\n", s, thread_cpu.reg32[ESP] + s, val, baddr);
        uw_free(baddr);
    }


    exit(1); // TODO: libnx won't handle it nicely
}

static void write_dt(void *ptr, unsigned long addr, unsigned long limit,
                     int flags)
{
    unsigned int e1, e2;
    uint32_t *p;
    e1 = (addr << 16) | (limit & 0xffff);
    e2 = ((addr >> 16) & 0xff) | (addr & 0xff000000) | (limit & 0x000f0000);
    e2 |= flags;
    p = ptr;
    p[0] = e1;
    p[1] = e2;
}


static uint32_t setup_teb(uw_target_thread_data_t *params) {
    params->teb_base = uw_target_map_memory_dynamic(uw_host_page_size, UW_PROT_RW);
    uint32_t *teb = g2h(params->teb_base);

    teb[UW_TEB_SEH_FRAME]       = 0xffffffff; //Current SEH frame
    teb[UW_TEB_STACK_BOTTOM]    = params->stack_top + params->stack_size;
    teb[UW_TEB_STACK_TOP]       = params->stack_top;
    teb[UW_TEB_PTEB]            = params->teb_base;
    teb[UW_TEB_INITLIST]        = params->process->library_init_list;
    teb[UW_TEB_PID]             = params->process->process_id;
    teb[UW_TEB_TID]             = params->thread_id;
    teb[UW_TEB_CMDLINE]         = params->process->command_line;
    teb[UW_TEB_LAST_ERROR]      = 0;

    // dirty stuff...
    win32_last_error = teb + UW_TEB_LAST_ERROR;

    return params->teb_base;
}

static uint32_t setup_stack(uw_target_thread_data_t *params) {
    // hacky hacks for stack protection =)
    uint32_t stack_guard_1 = uw_target_map_memory_dynamic(uw_host_page_size, UW_PROT_N);
    params->stack_top = uw_target_map_memory_dynamic(params->stack_size, UW_PROT_RW);
    uint32_t stack_guard_2 = uw_target_map_memory_dynamic(uw_host_page_size, UW_PROT_N);

    assert(stack_guard_1 != (uint32_t)-1 && params->stack_top != (uint32_t)-1 && stack_guard_2 != (uint32_t)-1);

    return params->stack_top;
}

#define DESC_G_SHIFT    23
#define DESC_G_MASK     (1 << DESC_G_SHIFT)
#define DESC_B_SHIFT    22
#define DESC_B_MASK     (1 << DESC_B_SHIFT)
#define DESC_L_SHIFT    21 /* x86_64 only : 64 bit code segment */
#define DESC_L_MASK     (1 << DESC_L_SHIFT)
#define DESC_AVL_SHIFT  20
#define DESC_AVL_MASK   (1 << DESC_AVL_SHIFT)
#define DESC_P_SHIFT    15
#define DESC_P_MASK     (1 << DESC_P_SHIFT)
#define DESC_DPL_SHIFT  13
#define DESC_DPL_MASK   (3 << DESC_DPL_SHIFT)
#define DESC_S_SHIFT    12
#define DESC_S_MASK     (1 << DESC_S_SHIFT)
#define DESC_TYPE_SHIFT 8
#define DESC_TYPE_MASK  (15 << DESC_TYPE_SHIFT)
#define DESC_A_MASK     (1 << 8)

void uw_cpu_setup_thread(void* cpu_context, uw_target_thread_data_t *params) {
    thread_cpu_ptr = cpu_context;

    cpu_init();
    cpu_init_32bit();

    thread_cpu.cr[0] = CR0_PG | CR0_WP | CR0_PE;
    thread_cpu.cr[4] = CR4_OSXMMEXCPT;

    {
        struct seg_desc *gdt_table;
        thread_cpu.seg_base[SEG_GDTR] = params->gdt_base = uw_target_map_memory_dynamic(
                UW_ALIGN_UP(sizeof(uint64_t) * TARGET_GDT_ENTRIES, uw_host_page_size),
                UW_PROT_RW
        );
        thread_cpu.seg_limit[SEG_GDTR] = (sizeof(uint64_t) * TARGET_GDT_ENTRIES - 1);

        gdt_table = g2h(thread_cpu.seg_base[SEG_GDTR]);


        // code and data (full address space)
        write_dt(&gdt_table[__USER_CS >> 3], 0, 0xfffff,
                 DESC_G_MASK | DESC_B_MASK | DESC_P_MASK | DESC_S_MASK |
                 (3 << DESC_DPL_SHIFT) | (0xa << DESC_TYPE_SHIFT));
        write_dt(&gdt_table[__USER_DS >> 3], 0, 0xfffff,
                 DESC_G_MASK | DESC_B_MASK | DESC_P_MASK | DESC_S_MASK |
                 (3 << DESC_DPL_SHIFT) | (0x2 << DESC_TYPE_SHIFT));

        // TEB
        uint32_t teb_base = setup_teb(params);
        write_dt(&gdt_table[__USER_FS >> 3], teb_base, 1,
                 DESC_G_MASK | DESC_B_MASK | DESC_P_MASK | DESC_S_MASK |
                 (3 << DESC_DPL_SHIFT) | (0x2 << DESC_TYPE_SHIFT));

        // tls! (only one page though). Can be implemented through TEB, but meh
        uint32_t tls_base = params->tls_base = uw_target_map_memory_dynamic(uw_host_page_size, UW_PROT_RW);
        write_dt(&gdt_table[__USER_GS >> 3], tls_base, 1,
                 DESC_G_MASK | DESC_B_MASK | DESC_P_MASK | DESC_S_MASK |
                 (3 << DESC_DPL_SHIFT) | (0x2 << DESC_TYPE_SHIFT));

#define LOD_SEG(seg, val) cpu_seg_load_protected(seg, val, &gdt_table[val >> 3])
        LOD_SEG(CS, __USER_CS);
        LOD_SEG(SS, __USER_DS);
        LOD_SEG(DS, __USER_DS);
        LOD_SEG(ES, __USER_DS);
        LOD_SEG(FS, __USER_FS);
        LOD_SEG(GS, __USER_GS);
#undef LOD_SEG

        thread_cpu.reg32[EAX] = params->entry_point;
        thread_cpu.reg32[EBX] = params->entry_param;
        thread_cpu.reg32[ESP] = setup_stack(params) + params->stack_size;
        SET_VIRT_EIP(params->process->init_entry);

    }
}

#define TICKS 65536

int cpu_interrupt(int vector, int error_code, int type, int eip_to_push)
{
    //uw_log("cpu_interrupt(%d, %d, %d, %d)\n", vector, error_code, type, eip_to_push);
    switch (vector) {
        case 0x80:
            thread_cpu.reg32[EAX] =
                    uw_cpu_do_syscall(
                            thread_cpu.reg32[EAX],
                            thread_cpu.reg32[EBX],
                            thread_cpu.reg32[ECX],
                            thread_cpu.reg32[EDX],
                            thread_cpu.reg32[ESI],
                            thread_cpu.reg32[EDI],
                            thread_cpu.reg32[EBP],
                            0, 0);
            SET_VIRT_EIP(eip_to_push);
            break;

        default:
        fault:
        {
            uint32_t pc = VIRT_EIP();
            fprintf(stderr, "qemu: 0x%08lx | %s: unhandled CPU exception 0x%x - aborting\n", (long) pc,
                    ldr_beautify_address((uint32_t) pc), vector);
            uw_cpu_panic("unhandled CPU exception");
            abort();
        }
    }
    return 0;
}

void uw_cpu_loop(void* cpu_context) {
    uint64_t cycle_count = 0;
    uint64_t start = uw_get_monotonic_time();
    uint64_t now = uw_get_monotonic_time();
    while (1) {

        pthread_mutex_lock(&uw_current_thread_data->suspend_mutex);
        while (uw_current_thread_data->suspend_request_count > 0) {
            uw_log("suspended\n");
            uw_current_thread_data->suspended = 1;
            pthread_cond_wait(&uw_current_thread_data->suspend_cond, &uw_current_thread_data->suspend_mutex);
        }
        if (uw_current_thread_data->suspended) {
            uw_log("resumed\n");
            uw_current_thread_data->suspended = 0;
        }

        pthread_mutex_unlock(&uw_current_thread_data->suspend_mutex);

        cycle_count += cpu_run(TICKS);
        now = uw_get_monotonic_time();
        if (cycle_count == UINT64_MAX)
            break;
    }

    uw_log("%lu %lu %lu\n", (unsigned long)start, (unsigned long)now, (unsigned long)cycle_count);
}
