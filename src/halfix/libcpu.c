// CPU library stubs
// If you want detailed reporting on CPU events, look into CPU instrumentation (see halfix/instrumentation.h)

#include "halfix/cpu/libcpu.h"
#include "halfix/cpu/cpu.h"
#include "halfix/cpuapi.h"
#include "halfix/state.h"
#include "halfix/util.h"

#ifdef _WIN32
#define EXPORT __declspec(dllexport)
#define IMPORT __declspec(dllimport)
#elif defined(EMSCRIPTEN)
#include <emscripten.h>
#define EXPORT EMSCRIPTEN_KEEPALIVE
#else
#define EXPORT __attribute__((visibility("default")))
#define IMPORT __attribute__((visibility("default")))
#endif

#define LIBCPUPTR_LOG(x, ...) LOG("LIBCPU", x, ##__VA_ARGS__)
#define LIBCPUPTR_DEBUG(x, ...) LOG("LIBCPU", x, ##__VA_ARGS__)
#define LIBCPUPTR_FATAL(x, ...) \
    FATAL("LIBCPU", x, ##__VA_ARGS__)

static void dummy(void)
{
    return;
}

static abort_handler onabort = dummy, pic_ack = dummy, fpu_irq = dummy;
static mem_refill_handler mrh = NULL, mrh_lin = NULL;
static int apic_enabled;


void util_abort(void)
{
    // Notify host and then abort
    onabort();
    cpu_debug();
    abort();
}

void state_register(state_handler s)
{
    UNUSED(s);
}

void* get_phys_ram_ptr(uint32_t addr, int write)
{
    if (mrh == NULL) {
        return g2h(addr);
    } else
        return mrh(addr, write);
}
void* get_lin_ram_ptr(uint32_t addr, int flags, int* fault)
{
    return g2h(addr);
    if (mrh_lin == NULL) {
        *fault = 0;
        return NULL;
    } else {
#define EXCEPTION_HANDLER return NULL
        void* dest = mrh_lin(addr, flags);
        *fault = dest == NULL;
        return dest;
#undef EXCEPTION_HANDLER
    }
}

void pic_lower_irq(void)
{
    // Do nothing
}
void pic_raise_irq(int dummy)
{
    UNUSED(dummy);
    fpu_irq();
}
EXPORT
void* cpu_get_state_ptr(int id)
{
    switch (id) {
    case CPUPTR_GPR:
        return thread_cpu.reg32;
    case CPUPTR_XMM:
        return thread_cpu.xmm32;
    case CPUPTR_MXCSR:
        return &thread_cpu.mxcsr;
    case CPUPTR_SEG_DESC:
        return thread_cpu.seg;
    case CPUPTR_SEG_LIMIT:
        return thread_cpu.seg_limit;
    case CPUPTR_SEG_BASE:
        return thread_cpu.seg_base;
    case CPUPTR_SEG_ACCESS:
        return thread_cpu.seg_access;
    case CPUPTR_MTRR_FIXED:
        return thread_cpu.mtrr_fixed;
    case CPUPTR_MTRR_VARIABLE:
        return thread_cpu.mtrr_variable_addr_mask;
    case CPUPTR_MTRR_DEFTYPE:
        return &thread_cpu.mtrr_deftype;
    case CPUPTR_PAT:
        return &thread_cpu.page_attribute_tables;
    case CPUPTR_APIC_BASE:
        return &thread_cpu.apic_base;
    case CPUPTR_SYSENTER_INFO:
        return thread_cpu.sysenter;
    }
    return NULL;
}
EXPORT
uint32_t cpu_get_state(int id)
{
    switch (id & 15) {
    case CPU_EFLAGS:
        return cpu_get_eflags();
    case CPU_EIP:
        return VIRT_EIP();
    case CPU_LINEIP:
        return LIN_EIP();
    case CPU_PHYSEIP:
        return PHYS_EIP();
    case CPU_CPL:
        return thread_cpu.cpl;
    case CPU_STATE_HASH:
        return thread_cpu.state_hash;
    case CPU_SEG:
        return thread_cpu.seg[id >> 4 & 15];
    case CPU_A20:
        return thread_cpu.a20_mask >> 20 & 1;
    case CPU_CR:
        return thread_cpu.cr[id >> 4 & 7];
    }
    return 0;
}
EXPORT
int cpu_set_state(int id, uint32_t value)
{
    switch (id & 15) {
    case CPU_EFLAGS:
        cpu_set_eflags(value);
        break;
    case CPU_EIP:
        SET_VIRT_EIP(value);
        break;
    case CPU_LINEIP:
        LIBCPUPTR_LOG("Cannot set linear EIP directly -- modify CS and EFLAGS\n");
        return 1;
    case CPU_PHYSEIP:
        thread_cpu.phys_eip = value;
        //halfix.eip_phys_bias = value ^ 0x1000;
        break;
    case CPU_CPL:
        thread_cpu.cpl = value & 3;
        cpu_prot_update_cpl();
        break;
    case CPU_STATE_HASH:
        thread_cpu.state_hash = value;
        break;
    case CPU_SEG:
        if (thread_cpu.cr[0] & CR0_PE) {
            if (thread_cpu.eflags & EFLAGS_VM)
                cpu_seg_load_virtual(id >> 4 & 15, value);
            else {
                struct seg_desc desc;
                if (cpu_seg_load_descriptor(value, &desc, EX_GP, value & 0xFFFC))
                    return 1;
                return cpu_seg_load_protected(id >> 4 & 15, value, &desc);
            }
        } else
            cpu_seg_load_real(id >> 4 & 15, value);
        break;
    case CPU_A20:
        cpu_set_a20(value & 1);
        break;
    case CPU_CR:
        return cpu_prot_set_cr(id >> 4 & 15, value);
    }
    return 0;
}

void cpu_init_32bit(void)
{
    thread_cpu.a20_mask = -1;
    thread_cpu.cr[0] = 1; // How you do paging is up to you
    // Initialize EFLAGS with IF
    thread_cpu.eflags = 0x202;
    // Set ESP to 32-bit
    thread_cpu.esp_mask = -1;
    // Set translation mode to 32-bit
    thread_cpu.state_hash = 0;
}

